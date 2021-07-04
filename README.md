# s-modem
SoapySDR Driver / Modem for Higgs.  We base our driver on the existing SoapySDR Driver format because it is simple to implement, it gives us a nice framework for how things should look, and it can connect to various open source sdr front-ends (gnuradio, redpitaya, and more)

# Suse Deps
Start with
```bash
sudo zypper install cmake boost-devel
```

# Ubuntu Deps
```bash
sudo apt-get install libboost-all-dev cmake
```

# Install
First you need to install SoapySDR.
```bash
git clone https://github.com/pothosware/SoapySDR.git
cd SoapySDR
mkdir build
cd build
cmake ..
make -j4
sudo make install
sudo ldconfig #needed on debian systems
SoapySDRUtil --info
```
(copied from https://github.com/pothosware/SoapySDR/wiki/BuildGuide)

Next install osmocom
```bash
git clone git://git.osmocom.org/gr-osmosdr
cd gr-osmosdr/
mkdir build
cd build/
cmake ../
make
sudo make install
sudo ldconfig
```

(Copied from https://github.com/osmocom/gr-osmosdr)

Now install zeromq (Ubuntu's version is BROKEN)
```bash
git clone https://github.com/zeromq/libzmq.git
cd libzmq
git checkout d062edd8c142384792955796329baf1e5a3377cd
mkdir build
cd build/
cmake ..
make
sudo make install

```

Install zeromq c++ bindings
```bash
git clone https://github.com/esromneb/cppzmq
cd cppzmq
mkdir build
cd build
cmake ..
make
sudo make install

```
(libevent) For suse `sudo zypper in openssl-devel`

Install libevent
```bash
sudo apt-get install libssl-dev
git clone https://github.com/libevent/libevent.git
cd libevent
git checkout e7ff4ef2b4fc950a765008c18e74281cdb5e7668
mkdir build
cd build
cmake -DEVENT__BUILD_SHARED_LIBRARIES=ON  ..
make
sudo make install
```


 Install eigen
In a place you will not delete (ie not downloads):
```
wget http://bitbucket.org/eigen/eigen/get/3.3.7.tar.gz
tar -zxf 3.3.7.tar.gz
sudo ln -s `realpath eigen-eigen-323c052e1731` /usr/local/include/eigen3
```
Note the name of the folder that comes out of the zip may be different


# Tun setup
For recent code you will need to install openvpn and setup a tun before running s-modem
```bash
sudo apt-get install openvpn -y
openvpn --mktun --dev tun1 --user ${USER}
ip link set tun1 up
ip addr add 10.2.4.1/24 dev tun1
```
Afterwards you can ping to start the experiment with:
```bash
ping -c 1 10.2.4.2
```

# Software Ubuntu
Run:
```bash
sudo apt-get install libjsonrpccpp-dev libjsonrpccpp-tools libmicrohttpd-dev libjsoncpp-dev libcurl4-openssl-dev gr-osmosdr
```


Now install this repo
```bash
git clone https://github.com/siglabsoss/s-modem
cd s-modem/soapy
mkdir build
cd build
cmake ..
make
make uninstall_link install_link
```

NOTE: `install_link` puts a symlink to the current build directory.  This means you can simply `make` and your system will use the fresh version of the Higgs Driver.  The downside is that if you have two copies of `s-modem` checked out, they will conflict.  Instead you can simply `sudo make install` which will do a normal file copy.

# Running
From the build directory you can run the generic soapy probe, (this runs a C++ equivelent of `make test_eth` under the hood)
```bash
make
SoapySDRUtil --probe="driver=HiggsDriver"
```

Also from the build diretory
```bash
make && ./modem_main
```

# Running under NodeJs
```bash
mkdir build
cd build
cmake ..
make -j
cd ..
cd productionjs

nvm use
npm i
npm run build

node modem_main.js
```

# Running Tests
You need to build cmake with like this.  Delete the build folder (or `rm -rf CMake* cmake*` from `build` ).  Then run this:
```bash
cmake -DBUILD_EXTRA=ON ..
make -j && ./test_ooo
```

Extras includes Discovery Server


# Troubleshooting
If you can't compile `s-modem`, double check that the cmake output does not say `The C compiler identification is GNU 4.8.5` and instead says `GNU 7.3.1` or higher.  See https://FIXME/wiki/Suse_Proxmox to fix this problem

# Running under GNU Radio
These steps will allow you to run Higgs as a GUI block in GNU Radio.
* Make sure that you got `osmocom` and all other steps from from above.
* Open GNU Radio Companion.
* Add a `osmocom Source` block.
  * Edit the block
  * Edit the `Device Arguments` field and put `driver=HiggsDriver`
* connect it and run

# Pre-made Gnu Radio Project
* See `grc/higgs-rx-example.grc`

# Documentation
This repository uses Doxygen to generate class documentation and hierarchical relations. To install Doxygen, please follow instructions [here](http://www.doxygen.nl/download.html). To generate HTML and LaTeX formatted documentation, execute

```bash
doxygen docs/s-modem-doxygen

# Alternately

make documentation
```

To view the resultant documentation on Chromium, execute
```bash
chromium-browser docs/html/index.html

# Alternately

make open_docs
```

# Contributing
To expand functions, you should see how SoapySDR does things.  List of example drivers, first are better:
* https://github.com/pothosware/SoapyRTLSDR
* https://github.com/pothosware/SoapyHackRF
* More under https://github.com/pothosware/

Also, see the driver guide https://github.com/pothosware/SoapySDR/wiki/DriverGuide 


# Soapy Builtins
After compiling and running install_link, you can query for the radio with:
* `SoapySDRUtil --probe="driver=HiggsDriver"`

# See Also
* Python example - https://github.com/myriadrf/LimeSuite/blob/master/SoapyLMS7/BasicStreamTests.py
* https://myriadrf.org/blog/limesdr-made-simple-part-8-c-examples-soapysdr-api/
* https://github.com/pothosware/SoapySDR/wiki/PythonSupport
* https://github.com/pothosware/SoapySDR/blob/master/python/apps/MeasureDelay.py


# Flow
* rxAsyncThread() runs and accepts sampels
  * it strips off our custom UDP sequence 4 bytes, and calls threadedAddDataToBuf with the rest
    * This calls _buf_cond.notify_one()
* user app builds objects
* user app calls setup
* user app calls readStream in a loop
  * readStream calls acquireReadBuffer() which uses a circular buffer as well as _buf_cond to get data
  * readStream converts this data to floats (or not) and then memcopies to user application
  * if readStream returns 0, user application must understand that no samples were there

# GNURadio Flow
GNURadio calls functions in this order:
* setupStream( direction = 1 #channels = 1 format = CF32 )
* setSampleRate()
* getSampleRate()
* setFrequency() 0
* getFrequency() 0
* setDCOffsetMode()
* setDCOffset()
* setGain() 0
* getGain() 0
* listGains()

Note that get/set Frequency/Gain have 2 overloads, (labeled 0 and 1)


# JsonCPP
Additional Ubuntu steps
To Setup:
```bash
sudo apt-get install libjsonrpccpp-dev libjsonrpccpp-tools libjsoncpp-dev libmicrohttpd-dev libcurl4-openssl-dev
```

