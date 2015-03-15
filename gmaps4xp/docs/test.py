#!/usr/bin/python
import wx

class Finestra(wx.Frame):
	
	def __init__(self):
		"""Costruttore della classe Finestra
		"""
		wx.Frame.__init__(self, None, -1, 'Ciao da wxPython',
			wx.DefaultPosition, wx.Size(220, 150))
		p = wx.Panel(self, -1)

		b = wx.Button(p, -1, 'Ciao mondo !')
		b.SetPosition((15, 15))
		
		self.Bind(wx.EVT_BUTTON, self.onHelloWorld, b)
		self.Bind(wx.EVT_CLOSE, self.onCloseWindow)

	def onHelloWorld(self, event):
		"""Stampa sullo standard output la stringa 'Hello world !'
		"""
		print 'Ciao mondo !'
		
	def onCloseWindow(self, event):
		"""Chiude l'applicazione alla chiusura della finestra
		"""
		self.Destroy()
		
	def mostra(self):
		"""Visualizza la finestra
		"""
		self.Show(True)


class Applicazione(wx.App):
	
	def OnInit(self):
		f = Finestra()
		f.mostra()
		self.SetTopWindow(f)
		return True

if __name__ == '__main__':
	a = Applicazione(0)
	a.MainLoop()

