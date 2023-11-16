# make the impinj_reader_chip_venv and install the required packages

# makes and activates the venv
python3 -m venv impinj_reader_chip_venv
source impinj_reader_chip_venv/bin/activate

echo "** installing requirements **"
pip install wheel
pip install -r impinj_reader_chip_venv-requirements.txt

echo "** installing impinj reader **"
pip install -e ../ex10_dev_kit

# copy the libgpiiod sources that were 'apt installed' into the venv
echo "** Installing python3-libgpiod **"
gpiod_src=/usr/lib/python3/dist-packages/gpiod.cpython-37m-arm-linux-gnueabihf.so
gpiod_dest=${VIRTUAL_ENV}/lib/python3.7/site-packages/
cp -v $gpiod_src $gpiod_dest

