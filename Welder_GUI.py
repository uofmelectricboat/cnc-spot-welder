import serial, serial.tools.list_ports
import time
import threading
import tkinter as tk
import customtkinter as ctk

finished = True
paused = False

ser = serial.Serial(baudrate=9600)
ser.port = 'COM5'
for port in serial.tools.list_ports.comports():
    print(port.name)
try:
    ser.open()
except serial.SerialException:
    print("Could not open port")

########################################################################
class MyApp(ctk.CTk):
    
    """"""
    
    def __init__(self, parent):
        """Constructor"""
        self.root = parent
        self.root.title("CNC Spot Welder GUI")
        
        self.frame = ctk.CTkFrame(parent)
        self.frame.pack(side=tk.TOP)

        self.titleText = ctk.CTkLabel(self.frame, text="CNC Spot Welder GUI")
        self.titleText.pack()
        
        self.body = ctk.CTkFrame(self.frame)
        self.body2 = ctk.CTkFrame(self.frame)
        self.body.pack(side=tk.TOP)
        self.body2.pack(side=tk.TOP)

        self.connectFrame = ctk.CTkFrame(self.body)
        self.connectFrame.pack(side=tk.TOP)

        self.connectText = ctk.CTkLabel(self.connectFrame, text="Connect to Arduino")
        self.connectText.pack(side=tk.LEFT)
        self.connectTargText = tk.StringVar()
        self.connectTarget = ctk.CTkEntry(self.connectFrame, textvariable=self.connectTargText)
        self.connectTarget.pack(side=tk.LEFT)
        self.connectionButton = ctk.CTkButton(self.connectFrame, text="Connect", command=self.connect)
        self.connectionButton.pack(side=tk.LEFT)

        self.homeButton = ctk.CTkButton(self.body, text="Home", command=self.home, state=tk.DISABLED)
        self.startButton = ctk.CTkButton(self.body, text="Start", command=self.start, state=tk.DISABLED)
        self.stopButton = ctk.CTkButton(self.body2, text="Stop", command=self.stop, state=tk.DISABLED)
        self.pauseButton = ctk.CTkButton(self.body2, text="Pause", command=self.pause, state=tk.DISABLED)

        self.titleText.pack()
        self.homeButton.pack(side=tk.LEFT)
        self.startButton.pack(side=tk.LEFT)
        self.stopButton.pack(side=tk.LEFT)
        self.pauseButton.pack(side=tk.LEFT)


    def connect(self):
        global ser
        ser = serial.Serial(baudrate=9600)
        ser.port = self.connectTargText.get()
        try:
            ser.open()
        except serial.SerialException:
            print("Could not open port")
            return;
        print(ser.is_open)
        self.startButton.configure(state=tk.NORMAL)
        self.homeButton.configure(state=tk.NORMAL)

    
    def hide(self):
        """"""
        self.root.withdraw()
        
    
    def openFrame(self):
        """"""
        self.hide()
        otherFrame = ctk.CTkToplevel()
        otherFrame.geometry("400x300")
        otherFrame.title("otherFrame")
        handler = lambda: self.onCloseOtherFrame(otherFrame)
        btn = ctk.CTkButton(otherFrame, text="Close", command=handler)
        btn.pack()
        
    
    def onCloseOtherFrame(self, otherFrame):
        """"""
        otherFrame.destroy()
        self.show()
        
    
    def show(self):
        """"""
        self.root.update()
        self.root.deiconify()

    def home(self):
        global finished
        global ser
        if not finished:
            return
        ser.write(b'homeAll\n')

    def start(self):
        global finished
        global paused
        global ser
        if paused:
            paused = False
            self.pauseButton.configure(state=tk.NORMAL)
            self.startButton.configure(state=tk.DISABLED)
            self.homeButton.configure(state=tk.DISABLED)
            self.stopButton.configure(state=tk.NORMAL)
            ser.write(b'continue\n')
            return
        if not finished:
            return
        finished = False
        self.stopButton.configure(state=tk.NORMAL)
        self.pauseButton.configure(state=tk.NORMAL)
        self.startButton.configure(state=tk.DISABLED)
        self.homeButton.configure(state=tk.DISABLED)
        ser.write(b'runScript\n')

    def stop(self):
        global finished
        global paused
        global ser
        finished = True
        paused = False
        self.stopButton.configure(state=tk.DISABLED)
        self.pauseButton.configure(state=tk.DISABLED)
        self.startButton.configure(state=tk.NORMAL)
        self.homeButton.configure(state=tk.NORMAL)
        ser.write(b'stop\n')

    def pause(self):
        global paused
        global ser
        paused = True
        self.pauseButton.configure(state=tk.DISABLED)
        self.startButton.configure(state=tk.NORMAL)
        self.homeButton.configure(state=tk.DISABLED)
        self.stopButton.configure(state=tk.NORMAL)
        ser.write(b'pause\n')
        

def finish(self):
    global finished
    finished = True
    
root = ctk.CTk()
root.geometry("800x600")
app = MyApp(root)

end = False

def checkFinish():
    global end
    while not end:
        time.sleep(0.1)
        if not ser.is_open:
            continue
        if (ser.in_waiting > 0):
            line = ser.readline()
            if (line == b'finished\n' or b'finished\r\n'):
                finish()

finishThread = threading.Thread(target=checkFinish)

finishThread.start()
root.mainloop()
end = True