;There was a problem - raise an error and quit

Local whatHappened$ = "Missing required items: frogs.mp3"
RuntimeError("Installation corrupted! Error - "+whatHappened+" Please reinstall.",1)