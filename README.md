# Projet RSA Keslick Darbon

## Sujet

Dans la cadre du TP/Projet, vous devez réaliser un client et un serveur UDP implantant un protocole de transfert de fichiers de type TFTP.
—  Le développement se fera par équipe de deux. Un des membres de l’équipe travaillera prioritairement sur le client et l’autre sur le serveur.
—  Vous livrerez le code source avec un Makefile permettant la génération des deux exécutables client etserveur et un fichier README donnant les indications pour l’exécution. Le développement doit se faireen langage C sur une machine UNIX. Il peut être réalisé en binôme. Il vous sera demandé de présenter une démonstration de votre travail et de pouvoir expliquer votre code.
—  Le client doit pouvoir demander d’écrire ou de lire un fichier sur un serveur. L’adresse IP du serveur, le numéro de port, le nom du fichier ainsi que le type d’opération (lecture ou écriture ) devront être fournis par l’utilisateur via la ligne de commande. Pour le projet, il vous est demandé d’implanter prioritairementla fonctionnalité de lecture (RRQ). La fonctionnalité d’écriture (WRQ) est facultative.
—  Afin de tester la fiabilité du transfert, vous intégrerez au niveau de votre code un mécanisme de simulation du taux de pertes des données et/ou des acquittements.
—  Le code doit pouvoir fonctionner en IPv4 et IPv6 (utilisation de getaddrinfo() ).
—  Vous n’avez pas à implanter le message ERROR de TFTP. De même, votre serveur tftp n’a pas à gérer plusieurs clients simultanément.

## Execution du code

Ouvrir deux terminaux, lancer un 'make' dans l'un des deux.
L'adresse IP, le numéro de port et le fichier à transmettre sont spécifiés au sein du Makefile : ils sont respectivement 127.0.0.1, 8080 et "data/test.txt".


### Serveur (responsable Frantz Darbon)

Lancer d'abord 'make run_serveur' dans l'un des deux terminaux.

### Client (responsable Malaury Keslick)

Puis lancer 'make run_client' dans l'autre terminal.

S'affichent alors dans chaque terminal les paquets envoyés et reçus par chacun des partis.
Lorsque la simulation est terminée, on retrouve le fichier transmis au client à la racine du dépôt, sous le nom de "output.txt".
