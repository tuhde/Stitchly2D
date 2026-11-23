# Guider on a Raspberry Pi 400

`guider` shows the written instructions as rolling fullscreen text and follows the row counter of a knitting machine.
This page sets up a Raspberry Pi 400 (Raspberry Pi OS with the X11 desktop) so that it starts guider by itself.

## 1. Build and install

```sh
sudo apt install build-essential qt6-base-dev qt6-svg-dev qt6-multimedia-dev qt6-l10n-tools \
    libxerces-c-dev libgpiod-dev pkg-config git
git clone <repository> Stitchly2D && cd Stitchly2D
mkdir build && cd build
qmake6 ../Stitchly2D.pro
make -j4            # the first build takes a while on a Pi; use -j3 if memory runs out
sudo make install   # guider goes to /usr/local/bin (use `qmake6 PREFIX=/usr ...` to change)
```

- The path to the build directory must not contain spaces.
- `libgpiod-dev` must be version 2.x for the GPIO row counter. Check with `pkg-config --modversion libgpiod`. If it is
  older or missing, guider still builds; the GPIO source then reports "no GPIO support" and the manual and simulated
  sources keep working. (Raspberry Pi OS Bookworm has 1.6, Trixie has 2.x; otherwise build libgpiod 2 from source.)
- Only guider is needed on the Pi. To build just it: `make -C src/libs -j4 && make -C src/app/guider`.
- Check it: `guider --windowed pattern.stitchly` on the desktop. `--help` lists all options.

## 2. Access to the GPIO pins

The user that runs guider must be in the `gpio` group (the default user of Raspberry Pi OS already is):

```sh
sudo usermod -aG gpio $USER     # log out and in again
gpiodetect                      # lists the chips; on the Pi 400 the header is usually gpiochip0
```

Set chip, line, edge and pull in guider's settings (the Settings button or key S) and use the Test button to see the pulses
arrive. USB counters are not supported yet.

## 3. Start at login

Raspberry Pi OS: `sudo raspi-config` > System Options > Boot / Auto Login > **Desktop Autologin**. Then use one of:

**a) Desktop autostart (simplest)**

```sh
mkdir -p ~/.config/autostart
cp share/guider/guider-autostart.desktop ~/.config/autostart/
```

**b) systemd user service (restarts guider after a crash)**

```sh
mkdir -p ~/.config/systemd/user
cp share/guider/guider.service ~/.config/systemd/user/
systemctl --user daemon-reload
systemctl --user enable --now guider.service
journalctl --user -u guider -f          # log
```

Use only one of the two. guider reopens the last pattern, piece and row. It starts fullscreen on the monitor it was
last on; the pointer is hidden.

## 4. Kiosk touches

- Turn off screen blanking: `raspi-config` > Display Options > Screen Blanking > No.
- Hide the panel if you do not want it: right-click the panel > Panel Settings > Advanced > Minimize panel when not
  in use. guider covers it in fullscreen anyway.
- Optional: copy `share/guider/guider.desktop` to `~/.local/share/applications/` for a menu entry.
- Different screen: guider sizes everything from the screen height. If text is too small or large on a small
  screen, set `GUIDER_UNIT=<pixels>` (a base unit, normally height / 54) in the service file.

## 5. Updating

`git pull && cd build && make -j4 && sudo make install`, then `systemctl --user restart guider` (or log out and in).
