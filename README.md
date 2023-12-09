# cnc-spot-welder

This project serves as a GUI to the CNC Spot Welder as developed for the University of Michigan Electric Boat project team.

Our welder is configured with XYZ control in order to weld our cells. They are in hexagonal configuration 24p16s with 1" spacing between.

All axes are driven by NEMA 17 stepper motors using drivers.

Current pin configuration (for Arduino Nano):
--
- 2: E-stop (uses internal pullup)
- 3: X Step
- 4: Y Step
- 5: Z Step
- 6: X Dir
- 7: Y Dir
- 8: Z Dir
- 9: X EN
- 10: Y EN
- 11: Z EN
- A0: X Home (internal pullup)
- A1: Y Home (internal pullup)
- A2: Z Home (internal pullup)

Requirements:
--
Arduino Libraries:
- AccelStepper
  
Python Libraries:
- tkinter
- customtkinter
- threading
- pyserial
- time