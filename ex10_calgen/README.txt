`content_template` directory stores all jinja2 and python files that generate CalGen blocks.
Jinja2 files will be the template and the python file will return a context map to fill in the jinja2 template.
`python_tools` dir stores CalGen helper python files.


<Running CalGen on Linux/Rpi>

1. Open terminal

2. Create a virtual environment
	- cd ex10_calgen
	- source source_me_for_calgen_venv.sh

3. Run the CalGen python script
	- python run_inline_calgen.py


<Running CalGen on Windows>

1. Open the command prompt

2. Create a virtual environment and activate it
	- cd ex10_calgen
	- py -m venv calgen_venv
	- calgen_venv\Scripts\activate.bat

3. Install CalGen package
	- pip install -e ..\ex10_calgen

4. Run the CalGen python script
	- py run_inline_calgen.py