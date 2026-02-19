import os
import os.path as osp
from threading import Thread
import udp_server as us
from udp_server import run_server, localIP, localPort, decode_message, TAG, DATA_FOLDER
import tkinter as tk
import time



def getControllers():
    return [x.removesuffix(".txt") for x in os.listdir(DATA_FOLDER)]


def main():
    server = Thread(target=run_server)
    server.start()

    time.sleep(1)

    root = tk.Tk()
    root.title("OBO message monitor")

    ipL = tk.Label(root, text=f"Server IP: {localIP}")
    ipL.grid(row=0, column=0, sticky=tk.W)

    portL = tk.Label(root, text=f"Server PORT: {localPort}")
    portL.grid(row=1, column=0, sticky=tk.W)

    controllerLabel = tk.Label(root, text=f"Select Controller:")
    controllerLabel.grid(row=2, column=0, sticky=tk.W)

    controllers = getControllers()
    print(controllers)

    cVar = tk.StringVar()
    cVar.set(controllers[0])
    controllerList = tk.OptionMenu(
        root,
        cVar,
        *controllers
    )
    controllerList.grid(row=2, column=1, sticky=tk.W)

    cL = tk.Label(root, text=f"Select Message No:")
    cL.grid(row=2, column=8, sticky=tk.W)
    options = ["all", "50", "20", "10"]
    mVar = tk.StringVar()
    mVar.set(options[1])
    msgList = tk.OptionMenu(
        root,
        mVar,
        *options
    )
    msgList.grid(row=2, column=9, sticky=tk.W)

    v = tk.Scrollbar(root, orient='vertical')
    v.grid(row=4, column=10, sticky=tk.N + tk.S)

    Output = tk.Text(root, height=40,
                     width=80,
                     bg="light yellow",
                     yscrollcommand=v.set)
    Output.tag_config("warning", foreground="red")
    Output.tag_config("normal", foreground="black")
    Output.tag_config("special", foreground="purple")
    Output.grid(row=4, column=0, padx=10, pady=10, columnspan=10)
    Output.config(state='disabled')

    v.config(command=Output.yview)

    def updateLog():
        fName = cVar.get() + ".txt"
        with open(osp.join("data", fName)) as fin:
            data = fin.read().split("\n")
            data.remove("")
            messages = [line.split(" ") for line in data]
        nrLines = mVar.get()
        Output.config(state="normal")
        Output.delete("1.0", "end")
        if nrLines != "all":
            messages = messages[-int(nrLines):]
        for line in messages:
            Output.insert("end", decode_message(line) + "\n", TAG[line[0]])
        Output.config(state='disabled')


    updater = tk.Button(root, text="Display/Update Log", command=updateLog)
    updater.grid(row=3, padx=10, pady=10, columnspan=11)

    def stop_server():
        us.stopThread = True
        root.destroy()

    def update_callback():
        updateLog()
        root.after(5000, update_callback)

    root.after(5000, update_callback)
    root.protocol("WM_DELETE_WINDOW", stop_server)
    root.mainloop()


if __name__ == '__main__':
    main()
