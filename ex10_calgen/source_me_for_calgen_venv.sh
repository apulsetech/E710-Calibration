# makes and activates the calgen_venv
python3 -m venv calgen_venv
source calgen_venv/bin/activate

# install required packages for calgen
echo "** installing calgen requirements **"
pip install -e ../ex10_calgen
