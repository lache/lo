import tkinter as tk
import tkinter.ttk as ttk
import tkinter.font as font
from subprocess import call
	
class Application(tk.Frame):
	def __init__(self, master=None):
		super().__init__(master)
		self.pack()
		self.create_widgets()

	def create_widgets(self):
		btn_font = font.Font(family='Helvetica', size=20, weight='bold')
	
		self.title_label = tk.Label(self, text = 'Laidoff', font=("Courier", 44))
		self.title_label.pack(side="top")
		
		self.power_btn = tk.Button(self)
		self.power_btn["text"] = "Power"
		self.power_btn["command"] = self.power
		self.power_btn["font"] = btn_font
		self.power_btn.pack(side="top")
		
		self.home_btn = tk.Button(self)
		self.home_btn["text"] = "Home"
		self.home_btn["command"] = self.home
		self.home_btn["font"] = btn_font
		self.home_btn.pack(side="top")
		
		self.force_stop_btn = tk.Button(self)
		self.force_stop_btn["text"] = "Force Stop"
		self.force_stop_btn["command"] = self.force_stop
		self.force_stop_btn["font"] = btn_font
		self.force_stop_btn.pack(side="top")
		
		self.start_btn = tk.Button(self)
		self.start_btn["text"] = "Start"
		self.start_btn["command"] = self.start
		self.start_btn["font"] = btn_font
		self.start_btn.pack(side="top")
		
		self.reboot_btn = tk.Button(self)
		self.reboot_btn["text"] = "Reboot"
		self.reboot_btn["command"] = self.reboot
		self.reboot_btn["font"] = btn_font
		self.reboot_btn.pack(side="top")
		
		#self.quit = tk.Button(self, text="QUIT", fg="red", command=root.destroy)
		#self.quit.pack(side="bottom")

	def power(self):
		call(["adb", "shell", "input", "keyevent", "KEYCODE_POWER"], shell=True)
		
	def home(self):
		call(["adb", "shell", "input", "keyevent", "KEYCODE_HOME"], shell=True)
		
	def force_stop(self):
		call(["adb", "shell", "am", "force-stop", "com.popsongremix.laidoff"], shell=True)
		
	def start(self):
		call(["adb", "shell", "am", "start", "-n", "com.popsongremix.laidoff/com.popsongremix.laidoff.LaidOffNativeActivity"], shell=True)
		
	def reboot(self):
		call(["adb", "reboot"], shell=True)

root = tk.Tk()
root.wm_title("Laidoff")
app = Application(master=root)
app.mainloop()


