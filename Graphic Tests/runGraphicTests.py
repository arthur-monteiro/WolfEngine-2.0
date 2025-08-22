import subprocess
import os
import win32gui, win32com.client, win32con
import pyautogui
import threading
import time
import psutil

def screenshot(windowTitle):
    global screenshotComparisonResult
    if windowTitle:
        hwnd = win32gui.FindWindow(None, windowTitle)
        if hwnd:
            # Idk why but these 2 lines are needed (https://stackoverflow.com/questions/14295337/win32gui-setactivewindow-error-the-specified-procedure-could-not-be-found)
            shell = win32com.client.Dispatch("WScript.Shell")
            shell.SendKeys('%')
            win32gui.SetForegroundWindow(hwnd)
            win32gui.SetWindowPos(hwnd, win32con.HWND_TOPMOST, 0, 0, 0, 0, win32con.SWP_NOSIZE)
            time.sleep(0.1)
            x, y, x1, y1 = win32gui.GetClientRect(hwnd)
            x, y = win32gui.ClientToScreen(hwnd, (x, y))
            x1, y1 = win32gui.ClientToScreen(hwnd, (x1 - x, y1 - y))
            im = pyautogui.screenshot(region=(x, y, x1, y1))

            im.save("graphicTestExecution.jpg")

            screenshotComparisonResult = open("graphicTestExecution.jpg", "rb").read() == open("referenceGraphicTest.jpg", "rb").read()
            
            if screenshotComparisonResult == True:
                os.remove("graphicTestExecution.jpg")

        else:
            print('Window not found!')
            screenshotComparisonResult = False
        
def runProcess(processName):
    subprocess.run(processName)

def runProcessTest(folder, processFullName, processShortName, windowTitle):
    print("Starting " + windowTitle)

    os.chdir(folder)

    p0 = threading.Thread(target=runProcess, args=(processFullName,))
    p0.start()
    time.sleep(5)

    p1 = threading.Thread(target=screenshot, args=(windowTitle,))
    p1.start()
    p1.join()

    for proc in psutil.process_iter():
        if proc.name() == processShortName:
            proc.kill()

    p0.join()

runProcessTest('../Hello Triangle', '"../x64/Debug - Graphic Tests/Hello Triangle.exe"', "Hello Triangle.exe", "Hello Triangle")
if screenshotComparisonResult == False:
    exit(1)

runProcessTest('../Compute Pass', '"../x64/Debug - Graphic Tests/Compute Pass.exe"', "Compute Pass.exe", "Compute Pass")
if screenshotComparisonResult == False:
    exit(1)

runProcessTest('../Variable Rate Shading', '"../x64/Debug - Graphic Tests/Variable Rate Shading.exe"', "Variable Rate Shading.exe", "Variable Rate Shading")
if screenshotComparisonResult == False:
    exit(1)

exit(0)