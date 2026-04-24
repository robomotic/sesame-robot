import tkinter as tk
from tkinter import ttk, messagebox
import json
import os
import ctypes

class SesameStudioApp:
    def __init__(self, root):
        self.root = root
        self.root.title("Sesame Studio")
        
        # Set Windows AppUserModelID to ensure taskbar icon shows correctly
        try:
            myappid = 'sesame.robot.studio.1.0'
            ctypes.windll.shell32.SetCurrentProcessExplicitAppUserModelID(myappid)
        except Exception:
            pass
        
        icon_path = os.path.join(os.path.dirname(__file__), "assets", "sesame-sticker2.png")
        if os.name == "nt" and os.path.exists(icon_path): # this crashes on some linux
            try:
                from PIL import Image, ImageTk
                self.icon_photo = ImageTk.PhotoImage(Image.open(icon_path))
                self.root.iconphoto(True, self.icon_photo)
            except Exception:
                try:
                    self.icon_photo = tk.PhotoImage(file=icon_path)
                    self.root.iconphoto(True, self.icon_photo)
                except Exception:
                    pass
        self.root.geometry("800x1300")
        self.root.configure(bg="#121212")

        self.bg_color = "#121212"
        self.fg_color = "#e0e0e0"
        self.accent_color = "#007acc"
        self.input_bg = "#333333"

        self.style = ttk.Style()
        self.style.theme_use('clam')
        self.style.configure("TLabel", background=self.bg_color, foreground=self.fg_color, font=("Segoe UI", 10))
        self.style.configure("TButton", background=self.accent_color, foreground="#ffffff", borderwidth=0, font=("Segoe UI", 10, "bold"))
        self.style.map("TButton", background=[("active", "#005a9e")])
        self.style.configure("TEntry", fieldbackground=self.input_bg, foreground=self.fg_color)
        
        self.main_frame = tk.Frame(root, bg=self.bg_color)
        self.main_frame.pack(fill=tk.BOTH, expand=True, padx=20, pady=20)

        self.btn_help = tk.Canvas(self.root, width=30, height=30, bg="#121212", highlightthickness=0, cursor="hand2")
        self.btn_help.place(relx=1.0, x=-20, y=10, anchor="ne")
        self.btn_help.create_oval(2, 2, 28, 28, fill="#444444", outline="#666666")
        self.btn_help.create_text(15, 15, text="?", fill="white", font=("Segoe UI", 14, "bold"))
        self.btn_help.bind("<Button-1>", lambda e: self.show_help())

        self.canvas_frame = tk.Frame(self.main_frame, bg=self.bg_color)
        self.canvas_frame.pack(fill=tk.X, pady=(0, 5))
        
        self.canvas = tk.Canvas(self.canvas_frame, width=760, height=1, bg="#000000", highlightthickness=0)
        self.canvas.pack(anchor="center")
        
        self.draw_robot_schematic()
        
        self.servo_vars = {} # []
        self.create_servo_inputs()

        self.controls_frame = tk.Frame(self.main_frame, bg=self.bg_color)
        self.controls_frame.pack(fill=tk.X, pady=10)

        tk.Label(self.controls_frame, text="Delay (ms):", bg=self.bg_color, fg=self.fg_color, font=("Segoe UI", 12)).pack(side=tk.LEFT, padx=(0, 10))
        self.delay_var = tk.StringVar(value="200")
        self.delay_entry = tk.Entry(self.controls_frame, textvariable=self.delay_var, width=10, bg=self.input_bg, fg=self.fg_color, insertbackground="white")
        self.delay_entry.pack(side=tk.LEFT)

        self.btn_add = tk.Button(self.controls_frame, text="+ ADD FRAME", command=self.add_frame, bg=self.accent_color, fg="white", font=("Segoe UI", 11, "bold"), padx=20, pady=5, relief=tk.FLAT)
        self.btn_add.pack(side=tk.RIGHT)
        
        self.btn_clear_code = tk.Button(self.controls_frame, text="Clear Code", command=self.clear_code, bg="#c0392b", fg="white", font=("Segoe UI", 10), padx=15, pady=5, relief=tk.FLAT)
        self.btn_clear_code.pack(side=tk.RIGHT, padx=10)

        tk.Label(self.main_frame, text="Generated Code:", bg=self.bg_color, fg=self.fg_color, font=("Segoe UI", 12, "bold")).pack(anchor="w", pady=(10, 5))
        
        self.code_text = tk.Text(self.main_frame, height=12, bg="#000000", fg="#569cd6", font=("Consolas", 10), insertbackground="white", bd=0, padx=10, pady=10)
        self.code_text.pack(fill=tk.BOTH, expand=True)

        if hasattr(self, 'servo_colors'):
            for i in range(8):
                self.code_text.tag_configure(f"servo_{i}", foreground=self.servo_colors[i])
        
        self.btn_copy = tk.Button(self.main_frame, text="Copy to Clipboard", command=self.copy_code, bg="#27ae60", fg="white", font=("Segoe UI", 10, "bold"), pady=8, relief=tk.FLAT)
        self.btn_copy.pack(fill=tk.X, pady=(10, 0))

        self.status_label = tk.Label(self.main_frame, text="Ready", bg=self.bg_color, fg="#888888", font=("Segoe UI", 9))
        self.status_label.pack(anchor="e", pady=(5,0))

    def draw_robot_schematic(self):
        try:
            from PIL import Image, ImageTk
            
            target_w, target_h = 740, 380
            current_y = 0
            
            if os.path.exists("sesame-topdown.png"):
                pil_img1 = Image.open("sesame-topdown.png")
                ratio1 = min(target_w / pil_img1.size[0], target_h / pil_img1.size[1])
                new_size1 = (int(pil_img1.size[0] * ratio1), int(pil_img1.size[1] * ratio1))
                pil_img1 = pil_img1.resize(new_size1, Image.Resampling.LANCZOS)
                self.robot_image1 = ImageTk.PhotoImage(pil_img1)
                self.canvas.create_image(380, current_y, image=self.robot_image1, anchor="n")
                current_y += new_size1[1]
            
            if os.path.exists("sesame-angle-guide.png"):
                pil_img2 = Image.open("sesame-angle-guide.png")
                ratio2 = min(target_w / pil_img2.size[0], target_h / pil_img2.size[1])
                new_size2 = (int(pil_img2.size[0] * ratio2), int(pil_img2.size[1] * ratio2))
                pil_img2 = pil_img2.resize(new_size2, Image.Resampling.LANCZOS)
                self.robot_image2 = ImageTk.PhotoImage(pil_img2)
                self.canvas.create_image(380, current_y, image=self.robot_image2, anchor="n")
                current_y += new_size2[1]
                
            self.canvas.config(height=current_y)

        except ImportError:
            try:
                y_pos = 0
                if os.path.exists("sesame-topdown.png"):
                    self.robot_image1 = tk.PhotoImage(file="sesame-topdown.png").subsample(5, 5)
                    self.canvas.create_image(380, y_pos, image=self.robot_image1, anchor="n")
                    y_pos += self.robot_image1.height()
                
                if os.path.exists("sesame-angle-guide.png"):
                    self.robot_image2 = tk.PhotoImage(file="sesame-angle-guide.png").subsample(5, 5)
                    self.canvas.create_image(380, y_pos, image=self.robot_image2, anchor="n")
                    y_pos += self.robot_image2.height()
                
                self.canvas.config(height=y_pos)
            except Exception as e:
                print(f"Image load error: {e}")
                self.canvas.create_text(380, 200, text=f"Error loading images: {e}", fill="red")
        except Exception as e:
            print(f"General image error: {e}")
            self.canvas.create_text(380, 200, text=f"Error: {e}", fill="orange")


    def create_servo_inputs(self):
        cx, cy = 380, 180 
        
        configs = [
            ("L1", cx - 100, cy - 100, "L1"), ("L3", cx - 220, cy - 120, "L3"), 
            ("R1", cx + 100, cy - 100, "R1"), ("R3", cx + 220, cy - 120, "R3"), 
            ("L2", cx - 100, cy + 100, "L2"), ("L4", cx - 220, cy + 120, "L4"), 
            ("R2", cx + 100, cy + 100, "R2"), ("R4", cx + 220, cy + 120, "R4")  
        ]
        
        self.servo_colors = {
            0: "#FFFFFF", 
            1: "#FF69B4", 
            2: "#9370DB", 
            3: "#FF2600", 
            4: "#00CED1", 
            5: "#FFA500", 
            6: "#32CD32", 
            7: "#FFF23B"  
        }


        self.servo_vars = {} # [None] * 8

        for s_id, x, y, label in configs:
            f = tk.Frame(self.canvas, bg="#000000")
            
            color = self.servo_colors.get(s_id, "#007acc")
            tk.Label(f, text=label, bg="#000000", fg=color, font=("Segoe UI", 12, "bold")).pack(side=tk.TOP)
            
            var = tk.StringVar(value="90")
            self.servo_vars[s_id] = var
            
            border_frame = tk.Frame(f, bg=color, padx=1, pady=1)
            border_frame.pack(side=tk.TOP)
            
            entry = tk.Entry(border_frame, textvariable=var, width=4, justify="center", bg="#333333", fg="white", 
                             font=("Consolas", 12), insertbackground="white", bd=0)
            entry.pack()
            
            self.canvas.create_window(x, y, window=f)

    def add_frame(self):
        try:
            delay = int(self.delay_var.get())
        except ValueError:
            messagebox.showerror("Error", "Delay must be a number")
            return

        self.code_text.insert(tk.END, "// Frame\n")
        
        def insert_command(servo_idx, angle):
            command_str = f"setServoAngle({servo_idx}, {angle}); "
            self.code_text.insert(tk.END, command_str, f"servo_{servo_idx}")

        for i in ["R1","L2"]:
            try:
                val = int(self.servo_vars[i].get())
                if val < 45 or val > 180: raise ValueError
                insert_command(i, val)
            except ValueError:
                messagebox.showerror("Error", f"Invalid angle for Servo {i}, range is 45 - 180")
                return
        
        for i in ["R2","L1"]:
            try:
                val = int(self.servo_vars[i].get())
                if val < 0 or val > 135: raise ValueError
                insert_command(i, val)
            except ValueError:
                messagebox.showerror("Error", f"Invalid angle for Servo {i}, range is 0 - 135")
                return
        
        self.code_text.insert(tk.END, "\n")
        for i in ["R3","R4","L3","L4"]:
            try:
                val = int(self.servo_vars[i].get())
                if val < 0 or val > 180: raise ValueError
                insert_command(i, val)
            except ValueError:
                messagebox.showerror("Error", f"Invalid angle for Servo {i}, range is 0 - 180")
                return
        self.code_text.insert(tk.END, "\n")
        
        self.code_text.insert(tk.END, f"delay({delay});\n\n")
        self.code_text.see(tk.END)
        self.status_label.config(text="Frame added successfully")

    def copy_code(self):
        code = self.code_text.get("1.0", tk.END)
        self.root.clipboard_clear()
        self.root.clipboard_append(code)
        self.status_label.config(text="Code copied to clipboard!")
        
    def clear_code(self):
        self.code_text.delete("1.0", tk.END)
        self.status_label.config(text="Code cleared")

    def show_help(self):
        help_win = tk.Toplevel(self.root)
        help_win.geometry(self.root.geometry())
        x = self.root.winfo_x()
        y = self.root.winfo_y()
        help_win.geometry(f"+{x}+{y}")
        help_win.title("Help Overlay")
        help_win.configure(bg="black")
        help_win.attributes('-alpha', 0.85) 
        
        help_win.overrideredirect(True)
        
        hc = tk.Canvas(help_win, bg="black", highlightthickness=0)
        hc.pack(fill=tk.BOTH, expand=True)
        
        arrow_color = "white"
        text_color = "white"
        font_main = ("Segoe UI", 14, "bold")
        font_desc = ("Segoe UI", 12)

        hc.create_line(150, 180, 200, 220, fill=arrow_color, width=3, arrow=tk.LAST)
        hc.create_text(150, 130, text="1. Set Angles", fill=text_color, font=font_main, anchor="s")
        hc.create_text(50, 150, text="Type 0-180 for each servo.\n90 is usually center/straight.", fill="#cccccc", font=font_desc, anchor="nw")

        hc.create_line(650, 520, 700, 720, fill=arrow_color, width=3, arrow=tk.LAST) 
        hc.create_text(650, 500, text="2. Add Frame", fill=text_color, font=font_main, anchor="s")
        hc.create_text(550, 520, text="Set delay (speed) and\nclick Add to save pose.", fill="#cccccc", font=font_desc, anchor="nw")

        hc.create_line(400, 920, 400, 950, fill=arrow_color, width=3, arrow=tk.LAST)
        hc.create_text(400, 900, text="3. Get Code", fill=text_color, font=font_main, anchor="s")
        hc.create_text(250, 920, text="Copy the C++ code to your\nfirmware sketch.", fill="#cccccc", font=font_desc, anchor="nw")
        
        hc.create_text(400, 50, text="CLICK ANYWHERE TO CLOSE", fill="#ff5555", font=("Segoe UI", 16, "bold"))

        help_win.bind("<Button-1>", lambda e: help_win.destroy())
        hc.bind("<Button-1>", lambda e: help_win.destroy())

if __name__ == "__main__":
    root = tk.Tk()
    app = SesameStudioApp(root)
    root.mainloop()
