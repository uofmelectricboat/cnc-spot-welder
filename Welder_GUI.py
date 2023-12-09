import serial, serial.tools.list_ports
import time
import threading
from PIL import Image
import tkinter as tk
import customtkinter as ctk

finished = True
paused = False

ser = serial.Serial(baudrate=9600)
ser.port = 'COM5'

########################################################################
class GUI(ctk.CTk):
    def __init__(self, parent):
        """Constructor"""
        self.root = parent
        self.root.geometry("500x400")
        
        self.root.title("CNC Spot Welder GUI")
        self.root.configure(fg_color="#08003A")
        
        self.titleFrame = ctk.CTkFrame(parent, fg_color="#08003A")
        self.titleFrame.pack(side=tk.TOP, fill=tk.X, pady=15)

        self.titleLeftSpacer = ctk.CTkFrame(self.titleFrame, fg_color="#08003A", width=30, height=50)
        self.titleLeftSpacer.pack(side=tk.LEFT, expand=False, anchor=tk.E)

        self.logoImg = Image.open("assets/logo.png")
        self.logoImg = ctk.CTkImage(self.logoImg, size=(75, 75))
        self.logo = ctk.CTkLabel(self.titleFrame, image=self.logoImg, text="", anchor=tk.W)
        self.logo.pack(side=tk.LEFT, anchor=tk.W)

        self.titleRightSpacer = ctk.CTkFrame(self.titleFrame, fg_color="#08003A", width=30, height=50)
        self.titleRightSpacer.pack(side=tk.RIGHT, expand=False, anchor=tk.E)

        self.titleText = ctk.CTkLabel(self.titleFrame, text="CNC Spot Welder GUI", font=("Berlin Sans FB", 36), anchor=tk.E)
        self.titleText.pack(side=tk.RIGHT, fill=tk.X, expand=True, anchor=tk.E)

        self.statusFrame = ctk.CTkFrame(parent, fg_color="#08003A")
        self.statusFrame.pack(side=tk.TOP, fill=tk.X, pady=20, padx=30)

        self.statusText = ctk.CTkLabel(self.statusFrame, text="Current Status:", font=("Berlin Sans FB", 18))
        self.statusText.pack(side=tk.LEFT, fill=tk.X, padx=10)
        self.statusCurrent = ctk.CTkLabel(self.statusFrame, text="Idle", text_color="yellow", font=("Berlin Sans FB", 18))
        self.statusCurrent.pack(side=tk.LEFT)
        self.startButton = ctk.CTkButton(self.statusFrame, text="Begin Welding", text_color="#08003A", command=self.start, state=tk.DISABLED, corner_radius=999, height=35, width=120)
        self.startButton.pack(side=tk.RIGHT, padx=20)

        self.body = ctk.CTkFrame(parent)
        self.body2 = ctk.CTkFrame(parent)
        # self.body.pack(side=tk.TOP)
        # self.body2.pack(side=tk.TOP)

        self.homeButton = ctk.CTkButton(self.body, text="Home", command=self.home, state=tk.DISABLED)
        self.stopButton = ctk.CTkButton(self.body2, text="Stop", command=self.stop, state=tk.DISABLED)
        self.pauseButton = ctk.CTkButton(self.body2, text="Pause", command=self.pause, state=tk.DISABLED)

        self.titleText.pack()
        # self.homeButton.pack(side=tk.LEFT)
        # self.stopButton.pack(side=tk.LEFT)
        # self.pauseButton.pack(side=tk.LEFT)

        # Connection frame at the bottom
        self.connectFrame = ctk.CTkFrame(parent, fg_color="#08003A")
        self.connectFrame.pack(side=tk.BOTTOM, pady=10, fill=tk.X)
        self.connectionRefreshButton = ctk.CTkButton(self.connectFrame, text="Refresh", command=self.refreshConnections, corner_radius=999, width=50, fg_color="green")
        self.connectionRefreshButton.pack(side=tk.LEFT, padx=5)
        self.connectTargText = tk.StringVar()
        self.connectTarget = ctk.CTkComboBox(self.connectFrame, variable=self.connectTargText, values=[port.name for port in serial.tools.list_ports.comports()])
        self.connectTarget.pack(side=tk.LEFT, padx=5)
        self.connectionButton = ctk.CTkButton(self.connectFrame, text="Connect", command=self.connect, width=80, corner_radius=999, fg_color="green")
        self.connectionButton.pack(side=tk.LEFT, padx=5)

    def refreshConnections(self):
        self.connectTarget['values'] = [port.name for port in serial.tools.list_ports.comports()]

    def connect(self):
        global ser
        ser = serial.Serial(baudrate=9600)
        ser.port = self.connectTargText.get()
        if (ser.is_open):
            ser.close()
        if (ser.port == ""):
            tk.messagebox.showerror("Connection Error", "No port selected")
            return;
        try:
            ser.open()
        except serial.SerialException:
            print("Could not open port")
            tk.messagebox.showerror("Connection Error", "Could not open port")
            return;
        print(ser.is_open)
        self.startButton.configure(state=tk.NORMAL)
        self.homeButton.configure(state=tk.NORMAL)

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
app = GUI(root)

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