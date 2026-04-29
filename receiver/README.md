source venv/bin/activate #use this to activate the virtual environment

This is the receiver portion of the project:

It connects to a USB port which has an STM32 board plugged in.

Receives a byte stream of data from



Here is my idea for the flow of the receiver:

1. Listen for a SOF byte