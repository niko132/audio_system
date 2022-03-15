sudo systemctl daemon-reload
sudo systemctl stop bluealsa-aplay
sudo systemctl stop bluealsa
sudo systemctl daemon-reload

cd bluez-alsa
rm -rf build/
autoreconf --install --force
mkdir build
cd build
../configure --enable-debug
make
sudo src/bluealsa -i hci0 -p a2dp-sink &
sudo utils/aplay/bluealsa-aplay -Dupmix2to51laptop --pcm-buffer-time=96000 --pcm-period-time=32000 --single-audio -vv 00:00:00:00:00:00
# top

