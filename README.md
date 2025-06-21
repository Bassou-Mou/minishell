# minishell
Objectif : Développer un mini-interpréteur de commandes type Unix (shell) en langage C dans le cadre du module Systèmes d'exploitation centralisés à l'ENSEEIHT.

Fonctionnalités principales :

⚙️ Exécution de commandes via fork() et execvp()

🔁 Boucle principale interactive avec parsing de commandes (readcmd)

📁 Redirections d'entrée/sortie avec < et >

🔗 Gestion des tubes (|) entre plusieurs commandes

👻 Exécution en arrière-plan avec & (et gestion des zombies)

🚫 Gestion des signaux SIGINT et SIGTSTP (Ctrl+C / Ctrl+Z)

🧩 Architecture modulaire et évolutive


Méthodologie de test :

✅ Commandes simples (ls, pwd, exit)

📤 Redirections (ls > fichier.txt, sort < fichier.txt)

🔗 Tubes combinés (cat fichier | grep mot | wc -l)

💤 Commandes en arrière-plan (sleep 50 &)

⛔ Tests robustes sur la gestion des signaux


📄 Technologies : C, Unix/Linux, readcmd.h
