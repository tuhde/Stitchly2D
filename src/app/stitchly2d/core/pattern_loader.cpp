#include "pattern_loader.h"

#include <QFileInfo>
#include <QScopedPointer>
#include <QSet>
#include <QtMath>

#include "../ifc/exception/vexception.h"
#include "../ifc/xml/multi_size_converter.h"
#include "../ifc/xml/individual_size_converter.h"
#include "../ifc/xml/vpatternconverter.h"
#include "../ifc/xml/vabstractpattern.h"
#include "../vformat/measurements.h"
#include "../vlayout/vlayoutpiece.h"
#include "../vmisc/def.h"
#include "../vpatterndb/measurements_def.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vpiece.h"
#include "../vtools/tools/vabstracttool.h"
#include "../vwidgets/vmaingraphicsscene.h"
#include "../vwidgets/vmaingraphicsview.h"

#include "application_2d.h"
#include "headless_window.h"
#include "../xml/vpattern.h"

namespace
{
QPointF toMm(const QPointF &px)
{
    return QPointF(FromPixel(px.x(), Unit::Mm), FromPixel(px.y(), Unit::Mm));
}

QPolygonF closedMm(const QVector<QPointF> &px)
{
    QPolygonF poly;
    for (const QPointF &p : px)
    {
        poly << toMm(p);
    }
    if (poly.size() > 1 && poly.first() != poly.last())
    {
        poly << poly.first();
    }
    return poly;
}

// The grainline path is: start, arrow, arrow, start, end, arrow, arrow, end (arrows are optional).
// The two ends are the pair of points that lie furthest apart.
QLineF grainlineFromPath(const QVector<QPointF> &px)
{
    qreal best = 0;
    QLineF line;
    for (int i = 0; i < px.size(); ++i)
    {
        for (int j = i + 1; j < px.size(); ++j)
        {
            const QLineF l(toMm(px.at(i)), toMm(px.at(j)));
            if (l.length() > best)
            {
                best = l.length();
                line = l;
            }
        }
    }
    return line;
}

bool fail(QString *error, const QString &msg)
{
    if (error) *error = msg;
    return false;
}
}

bool loadPattern(const QString &fileName, LoadedPattern &out, QString *error, const QString &measurementsOverride)
{
    if (!QFileInfo::exists(fileName))
    {
        return fail(error, QStringLiteral("file not found: %1").arg(fileName));
    }

    // Everything the Seamly2D parser expects to find. Heap objects are owned by the scope guards below,
    // in reverse order of creation, so the document goes before the scenes it points to.
    QScopedPointer<HeadlessWindow>       window(new HeadlessWindow());
    QScopedPointer<VMainGraphicsScene>   draftScene(new VMainGraphicsScene());
    QScopedPointer<VMainGraphicsScene>   pieceScene(new VMainGraphicsScene());
    QScopedPointer<VMainGraphicsView>    view(new VMainGraphicsView());
    view->setScene(draftScene.data());
    QGraphicsScene *currentScene = draftScene.data();

    qApp->setPatternUnit(Unit::Cm);
    qApp->setPatternType(MeasurementsType::Unknown);
    QScopedPointer<VContainer> data(new VContainer(qApp->translateVariables(), qApp->patternUnitP()));
    QScopedPointer<VPattern>   doc(new VPattern(data.data(), draftScene.data(), pieceScene.data()));

    qApp->setMainWindow(window.data());
    qApp->setCurrentScene(&currentScene);
    qApp->setSceneView(view.data());
    qApp->setCurrentDocument(doc.data());
    qApp->setCurrentData(data.data());
    qApp->setFilePath(fileName);

    try
    {
        qApp->setOpeningPattern();
        VPatternConverter converter(fileName);
        doc->setXMLContent(converter.Convert());
        qApp->setPatternUnit(doc->measurementUnits());

        // Measurements
        QString mPath = measurementsOverride;
        if (mPath.isEmpty() && !doc->MPath().isEmpty())
        {
            mPath = AbsoluteMPath(fileName, doc->MPath());
        }
        QScopedPointer<MeasurementDoc> measurements;
        if (!mPath.isEmpty())
        {
            if (!QFileInfo::exists(mPath))
            {
                return fail(error, QStringLiteral("measurement file not found: %1").arg(mPath));
            }
            measurements.reset(new MeasurementDoc(data.data()));
            measurements->setSize(VContainer::rsize());
            measurements->setHeight(VContainer::rheight());
            measurements->setXMLContent(mPath);
            if (measurements->Type() == MeasurementsType::Unknown)
            {
                return fail(error, QStringLiteral("unknown measurement file type: %1").arg(mPath));
            }
            if (measurements->Type() == MeasurementsType::Multisize)
            {
                MultiSizeConverter conv(mPath);
                measurements->setXMLContent(conv.Convert());
            }
            else
            {
                IndividualSizeConverter conv(mPath);
                measurements->setXMLContent(conv.Convert());
            }
            if (!measurements->eachKnownNameIsValid())
            {
                return fail(error, QStringLiteral("invalid measurement names in %1").arg(mPath));
            }
            // The pattern must not use measurements the file does not have.
            const QStringList haveList = measurements->ListAll();
            const QSet<QString> have(haveList.cbegin(), haveList.cend());
            QStringList missing;
            for (const QString &name : doc->ListMeasurements())
            {
                if (!have.contains(name)) missing << name;
            }
            if (!missing.isEmpty())
            {
                missing.sort();
                return fail(error, QStringLiteral("measurements missing in %1: %2").arg(mPath, missing.join(", ")));
            }
            qApp->setPatternType(measurements->Type());
            data->ClearVariables(VarType::Measurement);
            measurements->readMeasurements();
            if (measurements->Type() == MeasurementsType::Multisize)
            {
                VContainer::setSize(UnitConvertor(measurements->BaseSize(), measurements->measurementUnits(),
                                                  *measurements->GetData()->GetPatternUnit()));
                VContainer::setHeight(UnitConvertor(measurements->BaseHeight(), measurements->measurementUnits(),
                                                    *measurements->GetData()->GetPatternUnit()));
            }
            else
            {
                const QHash<QString, QSharedPointer<VInternalVariable>> *vars = data->DataVariables();
                VContainer::setSize(vars->contains(size_M) ? *vars->value(size_M)->GetValue() : 0);
                VContainer::setHeight(vars->contains(height_M) ? *vars->value(height_M)->GetValue() : 0);
            }
            out.measurementsFile = mPath;
        }

        doc->Parse(Document::FullParse);

        out.fileName = fileName;
        out.pieces.clear();
        const QHash<quint32, VPiece> *pieces = data->DataPieces();
        for (auto it = pieces->cbegin(); it != pieces->cend(); ++it)
        {
            const VPiece &piece = it.value();
            VAbstractTool *tool = qobject_cast<VAbstractTool *>(VAbstractPattern::getTool(it.key()));
            const VContainer *pieceData = tool != nullptr ? tool->getData() : data.data();
            const VLayoutPiece layout = VLayoutPiece::Create(piece, pieceData);
            LoadedPiece lp;
            lp.name = piece.GetName();
            lp.seamLine = closedMm(layout.getContourPoints());
            lp.cutLine = closedMm(layout.getSeamAllowancePoints());
            lp.grainline = grainlineFromPath(layout.getGrainline());
            out.pieces.append(lp);
        }
    }
    catch (const VException &e)
    {
        return fail(error, e.ErrorMessage());
    }
    catch (const std::exception &e)
    {
        return fail(error, QString::fromLocal8Bit(e.what()));
    }

    qApp->setCurrentDocument(nullptr);
    qApp->setCurrentData(nullptr);
    qApp->setMainWindow(nullptr);
    return true;
}
