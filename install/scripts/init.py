# Called at DarkRadiant startup

t = Temp()
t.printToConsole()

# t.print('Bla - Python is working!');

hello = file('hello.txt', 'w')
hello.write('Done.')
hello.close()
