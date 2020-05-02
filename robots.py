from Tkinter import *
 
#https://subscription.packtpub.com/book/web_development/9781788622301/1/ch01lvl1sec20/handling-mouse-and-keyboard-events
class App(Tk): 
    def __init__(self): 
        pass
        super().__init__() 
        '''
        frame = tk.Frame(self, bg="green", height=100, width=100) 
        frame.bind("<Button-1>", self.print_event) 
        frame.bind("<Double-Button-1>", self.print_event) 
        frame.bind("<ButtonRelease-1>", self.print_event) 
        frame.bind("<B1-Motion>", self.print_event) 
        frame.bind("<Enter>", self.print_event) 
        frame.bind("<Leave>", self.print_event) 
        frame.pack(padx=50, pady=50) 
        '''
    '''
    def print_event(self, event): 
    position = "(x={}, y={})".format(event.x, event.y) 
    print(event.type, "event", position) 
    '''
 
master = Tk()

width = 48
height = 32

box_size = 20
canvas_width = width * box_size
canvas_height = height * box_size

color="#CCCCCC"

w = Canvas(master, 
           width=canvas_width,
           height=canvas_height,
           borderwidth=0, highlightthickness=0)
w.pack()

def print_background():
    for i in range( 0, width / 2 ):
        for k in range( 0, height ):
            j = i * 2 +( k % 2 )
            x1 = j * box_size
            y1 = k * box_size
            x2 = x1 + box_size
            y2 = y1 + box_size
            w.create_rectangle( x1, y1, x2, y2, fill=color, outline=color)

print_background()

#https://stackoverflow.com/questions/29158220/tkinter-understanding-mainloop
#mainloop()
master.update_idletasks()
master.update()

#while True:
#    pass
