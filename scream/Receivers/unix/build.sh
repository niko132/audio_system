rm -r build
mkdir build && cd build
cmake ..
make
./scream -u -p 4011 -i eth0 -o alsa -d upmix4to51laptopnormal -t 100 -v
