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
        ########################################################################
        # Initialize the root window
        self.root = parent
        self.root.geometry("500x400")
        
        self.root.title("CNC Spot Welder GUI")
        self.root.configure(fg_color="#08003A")

        ########################################################################
        # Title frame at the top
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

        ########################################################################
        # Current status frame
        # When status changes, modify self.statusCurrent
        self.statusFrame = ctk.CTkFrame(parent, fg_color="#08003A")
        self.statusFrame.pack(side=tk.TOP, fill=tk.X, pady=20, padx=30)

        self.statusText = ctk.CTkLabel(self.statusFrame, text="Current Status:", font=("Berlin Sans FB", 18))
        self.statusText.pack(side=tk.LEFT, fill=tk.X, padx=10)
        self.statusCurrent = ctk.CTkLabel(self.statusFrame, text="Idle", text_color="yellow", font=("Berlin Sans FB", 18))
        self.statusCurrent.pack(side=tk.LEFT)
        self.startButton = ctk.CTkButton(self.statusFrame, text="Begin Welding", text_color="#08003A", command=self.start, state=tk.DISABLED, corner_radius=999, height=35, width=120)
        self.startButton.pack(side=tk.RIGHT, padx=20)

        ########################################################################
        # Control frame
        self.controlFrame = ctk.CTkFrame(parent, fg_color="#08003A", width=100, height=75)
        self.controlFrame.pack(side=tk.TOP, pady=20, padx=20, fill=tk.NONE, anchor=tk.NW)
        
        self.yLeftButton = ctk.CTkButton(self.controlFrame, text="<", command=self.yLeft, corner_radius=999, width=25, height=25)
        self.yRightButton = ctk.CTkButton(self.controlFrame, text=">", command=self.yRight, corner_radius=999, width=25, height=25)
        self.yStepSizeVal = 0
        self.yLeftButton.grid(column=0, row=0, rowspan=3, padx=2, pady=2)
        self.yRightButton.grid(column=1, row=0, rowspan=3, padx=2, pady=2)
        self.yStepSize = ctk.CTkEntry(self.controlFrame, placeholder_text="Y Step", width=60)
        self.yStepSize.grid(column=0, columnspan=2, row=3, pady=10, padx=2)
        self.root.bind("<Left>", self.yLeft)
        self.root.bind("<Right>", self.yRight)

        self.placeholder1 = ctk.CTkFrame(self.controlFrame, fg_color="#08003A", width=10, height=50)
        self.placeholder1.grid(column=2, row=0, rowspan=3, padx=2, pady=2)

        self.zUpButton = ctk.CTkButton(self.controlFrame, text="^", command=self.zUp, corner_radius=999, width=25, height=25)
        self.zDownButton = ctk.CTkButton(self.controlFrame, text="v", command=self.zDown, corner_radius=999, width=25, height=25)
        self.zStepSizeVal = 0
        self.zUpButton.grid(column=3, row=0, padx=2, pady=2)
        self.zDownButton.grid(column=3, row=2, padx=2, pady=2)
        self.zStepSize = ctk.CTkEntry(self.controlFrame, placeholder_text="Z Step", width=60)
        self.zStepSize.grid(column=3, row=3, pady=10, padx=2)
        self.root.bind("<Up>", self.zUp)
        self.root.bind("<Down>", self.zDown)

        self.placeholder2 = ctk.CTkFrame(self.controlFrame, fg_color="#08003A", width=10, height=50)
        self.placeholder2.grid(column=4, row=0, rowspan=3, padx=2, pady=2)
        
        self.homeYButton = ctk.CTkButton(self.controlFrame, text="Home Y", command=self.homeY, width=50, height=30, corner_radius=999)
        self.homeYButton.grid(column=10, row=0, columnspan=2, padx=2, pady=5)
        self.homeZButton = ctk.CTkButton(self.controlFrame, text="Home Z", command=self.homeZ, width=50, height=30, corner_radius=999)
        self.homeZButton.grid(column=10, row=2, columnspan=2, padx=2, pady=5)
        self.homeAllButton = ctk.CTkButton(self.controlFrame, text="Home All", command=self.homeAll, width=50, height=30, corner_radius=999)
        self.homeAllButton.grid(column=10, row=3, columnspan=2, padx=2, pady=5)


        ########################################################################
        # Active script frame
        self.runnningFrame = ctk.CTkFrame(parent, fg_color="#08003A")
        
        self.stopButton = ctk.CTkButton(self.runnningFrame, text="Stop", command=self.stop)
        self.pauseButton = ctk.CTkButton(self.runnningFrame, text="Pause", command=self.pause)

        self.stopButton.pack(side=tk.LEFT)
        self.pauseButton.pack(side=tk.LEFT)

        ########################################################################
        # Connection frame at the bottom
        self.connectFrame = ctk.CTkFrame(self.root, fg_color="#08003A")
        self.connectFrame.pack(side=tk.BOTTOM, pady=10, fill=tk.X)
        self.connectionRefreshButton = ctk.CTkButton(self.connectFrame, text="Refresh", command=self.refreshConnections, corner_radius=999, width=50, fg_color="green")
        self.connectionRefreshButton.pack(side=tk.LEFT, padx=5)
        self.connectTargText = tk.StringVar()
        self.connectTarget = ctk.CTkComboBox(self.connectFrame, variable=self.connectTargText, values=[port.name for port in serial.tools.list_ports.comports()])
        self.connectTarget.pack(side=tk.LEFT, padx=5)
        self.connectionButton = ctk.CTkButton(self.connectFrame, text="Connect", command=self.connect, width=80, corner_radius=999, fg_color="green")
        self.connectionButton.pack(side=tk.LEFT, padx=5)


        ########################################################################
        # Prepare for user input
        self.disableControl()
        self.startButton.configure(state=tk.DISABLED)
        self.pauseButton.configure(state=tk.DISABLED)
        self.stopButton.configure(state=tk.DISABLED)

    
    def enableControl(self):
        self.yLeftButton.configure(state=tk.NORMAL)
        self.yRightButton.configure(state=tk.NORMAL)
        self.zUpButton.configure(state=tk.NORMAL)
        self.zDownButton.configure(state=tk.NORMAL)
        self.homeYButton.configure(state=tk.NORMAL)
        self.homeZButton.configure(state=tk.NORMAL)
        self.homeAllButton.configure(state=tk.NORMAL)

    def disableControl(self):
        self.yLeftButton.configure(state=tk.DISABLED)
        self.yRightButton.configure(state=tk.DISABLED)
        self.zUpButton.configure(state=tk.DISABLED)
        self.zDownButton.configure(state=tk.DISABLED)
        self.homeYButton.configure(state=tk.DISABLED)
        self.homeZButton.configure(state=tk.DISABLED)
        self.homeAllButton.configure(state=tk.DISABLED)
    
    def yLeft(self, args=0):
        global ser
        if not ser.is_open or not self.yLeftButton['state'] == tk.NORMAL:
            return
        if (self.yStepSize.get() == ""):
            tk.messagebox.showerror("Error", "No step size selected")
            return
        self.yStepSizeVal = self.yStepSize.get()
        ser.write(f'yMove {self.yStepSizeVal}\n'.encode('utf-8'))

    def yRight(self, args=0):
        global ser
        if not ser.is_open or not self.yRightButton['state'] == tk.NORMAL:
            return
        if (self.yStepSize.get() == ""):
            tk.messagebox.showerror("Error", "No step size selected")
            return
        self.yStepSizeVal = self.yStepSize.get()
        ser.write(f'yMove -{self.yStepSizeVal}\n'.encode('utf-8'))

    def zUp(self, args=0):
        global ser
        if not ser.is_open or not self.zUpButton['state'] == tk.NORMAL:
            return
        if (self.zStepSize.get() == ""):
            tk.messagebox.showerror("Error", "No step size selected")
            return
        self.zStepSizeVal = self.zStepSize.get()
        ser.write(f'zMove {self.zStepSizeVal}\n'.encode('utf-8'))

    def zDown(self, args=0):
        global ser
        if not ser.is_open or not self.zDownButton['state'] == tk.NORMAL:
            return
        if (self.zStepSize.get() == ""):
            tk.messagebox.showerror("Error", "No step size selected")
            return
        self.zStepSizeVal = self.zStepSize.get()
        ser.write(f'zMove -{self.zStepSizeVal}\n'.encode('utf-8'))

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
        self.enableControl()

    def homeY(self):
        global finished
        global ser
        if not finished:
            return
        ser.write(b'homeY\n')

    def homeZ(self):
        global finished
        global ser
        if not finished:
            return
        ser.write(b'homeZ\n')

    def homeAll(self):
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
            self.stopButton.configure(state=tk.NORMAL)
            self.disableControl()
            ser.write(b'continue\n')
            return
        if not finished:
            return
        finished = False
        self.stopButton.configure(state=tk.NORMAL)
        self.pauseButton.configure(state=tk.NORMAL)
        self.startButton.configure(state=tk.DISABLED)
        self.disableControl()
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
        self.enableControl()
        ser.write(b'stop\n')

    def pause(self):
        global paused
        global ser
        paused = True
        self.pauseButton.configure(state=tk.DISABLED)
        self.startButton.configure(state=tk.NORMAL)
        self.enableControl()
        self.stopButton.configure(state=tk.NORMAL)
        ser.write(b'pause\n')
    
    def lostConnection(self):
        self.disableControl()
        self.startButton.configure(state=tk.DISABLED)
        self.pauseButton.configure(state=tk.DISABLED)
        self.stopButton.configure(state=tk.DISABLED)
        self.statusCurrent.configure(text="Lost Connection", text_color="red")
        tk.messagebox.showerror("Connection Error", "Connection lost")
        

def finish(self):
    global finished
    finished = True
    
root = ctk.CTk()
app = GUI(root)

end = False
wasOpen = False

def checkFinish():
    global end
    global wasOpen
    while not end:
        time.sleep(0.1)
        if not ser.is_open:
            if wasOpen:
                app.lostConnection()
                wasOpen = False
            continue
        wasOpen = True
        if (ser.in_waiting > 0):
            line = ser.readline()
            if (line == b'finished\n' or b'finished\r\n'):
                finish()

finishThread = threading.Thread(target=checkFinish)

finishThread.start()
root.mainloop()
end = True