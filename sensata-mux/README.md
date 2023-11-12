# Sensata testing and logging code

## Setup:

You must have the arduino IDE with teensyduino in order to edit the teensy code. Go to [the PJRC documentation tutorial page](https://www.pjrc.com/teensy/tutorial.html) for info on how to set this up.

To log serial data using the python script `log_serial.py`, you must have the module `pyserial` [installed](https://pyserial.readthedocs.io/en/latest/pyserial.html#installation) in your python environment.

Displaying the data uses numpy, pandas, and matplotlib. Install these modules if you don't have them. Optionally you can use jupyter lab too.

## How to log data:

Using the arduino IDE, load `sensata_test.ino` onto the teensy. Make sure the serial monitor is NOT open in the IDE if you want to save the log. Only one program can access the serial at a time, so if the monitor is open anywhere else, then the logger won't work.

Once the teensy is powered, connected, and running `sensata-test.ino`, edit the first line of `log_serial.py` to set `port = ` to a string containing the name of the serial port where the teensy is connected. You can find this name by running `python -m serial.tools.list_ports` in your terminal, assuming you already have `pyserial` installed.

Finally, execute `python log_serial.py` from the command line (make sure your working directory is this folder, i.e. `..../teensy-onboard/sensata-mux/`). This will immediately begin logging to `log.txt`, also in this folder. Running the file will overwrite any previous log, so if you want to save these, do it before rerunning the logger.

To stop logging, press `Ctrl+C` (this will KeyboardInterrupt the python script).

## How to display data:

To display the data in a graph, you can run `display.py`. This assumes there are two sensors are named `0` and `1`, and that messages are logged in the form `[sensor name], [raw value]`. You can also run this in an interactive jupyter lab environment if you have jupyter lab installed by running `jupyter lab` from command line and opening `display.ipynb`.

If any of this doesn't work, contact me (Adrian).
