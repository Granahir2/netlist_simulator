# Simulateur de netlist

Le makefile fourni a pour cible par défaut la création de l'exécutable `netlist_simulator`.
Une cible `clean` est également fournie.

L'utilisation de `netlist_simulator` est la suivante :
```
netlist_simulator NETLIST [OPTIONS...]
```
où chaque option est soit :

- `-n<x>` qui implique que la netlist sera exécutée pendant x cycles

- `-i<x>` qui spécifie un fichier x utilisé comme entrée à la place de l'entrée standard.
Remarquons que ce fichier est un fichier binaire, dont chaque paquet de 64 bits représente
une entrée. Les entrées sont disposées par ordre de déclaration dans la netlist puis selon
le cycle d'exécution concerné.

- `-D<x>` cette option active la création d'un fichier journal x de débogage de la netlist, contenant
notamment le résultat du parsage, du scheduling ainsi que les tables de symboles. Si cette option
est activée sans qu'un fichier journal ne soit spécifier les informations sont exfiltrées sur `clog`.

- `-o<x>` qui stipule que la sortie `x` de la netlist sera affichée après chaque cycle de simulation.
Si aucune option n'est spécifiée, par défaut toutes les sorties sont affichées, dans l'ordre de définition
dans la netlist.

- `-q` change l'option par défaut précédente : si `-q` est présent et aucune spécification `-o` n'apparaît
alors la sortie ne comporte aucune des sorties de la netlist.

- `-w<x>` fait afficher les `x` premiers octets de la première RAM comme des caractères ASCII lors de chaque cycle de simulation.

- `-r<x>` charge le début du fichier `x` dans une ROM de la netlist. Si la netlist admet plusieurs ROMs,
elles sont chargées dans l'ordre de spécification.

- `-g<x>` charge la chaîne terminée par '\\0' x dans la RAM. Seule charger la première RAM est supportée.
Cela a pour but de passer des arguments complexes à la netlist.

- `-c<x>` enregistre l'entrée `x` dans le système d'horloge : à chaque seconde, l'entrée sera asservie de force
pendant un cycle. Cette option a priorité sur l'entrée. 

- `-f` bascule le système d'horloge en mode rapide : les entrées enregistrées par `-c` sont tout le temps asservies.

Si plusieurs options autres que `-o` sont spécifiées c'est la dernière qui fait foi. Remarquons que les chemins
et les entiers sont _affixés_ sans espace.
En cas d'exécution normale l'exécutable renvoie 0; il renvoie 1 si n'importe quelle erreur est rencontrée,
y compris si la netlist n'est pas valide.

## Rapport (8/11/23)

Étant donné la simplicité du format de netlist, le parseur est directement implémenté. Toutes les instructions
sont encodées par un type unique d'instruction, une énumération permettant de leur donner de la sémantique ainsi
qu'à leurs arguments.

L'entrée est vérifiée être bien typée par l'exécutable. C'est nécessaire afin d'éviter des bogues subtiles au vu de la méthode
de simulation ainsi que la sûreté du simulateur.

Le parsage d'une RAM émet plusieurs objets parsés :
une instruction RAMFETCH qui correspond à la lecture de la RAM et qui dépend de l'addresse de lecture
mais aussi une entrée dans une liste de RAM qui indique quels fils définissent les entrées pertinentes de la RAM
en écrite (WE, addresse/donnée d'écriture). Une construction similaire est faite sur les ROM.
Ce décodage permet d'implémenter naïvement l'ordonnancement des instructions; les écritures sur les RAM
ne sont pas des instructions et la simulation proprement dite peut s'en occuper à la fin de chaque cycle, respectant
la sémantique.

Pour la simulation, toutes les variables sont réalisées par des entiers 64 bits. Cela permet aux instructions
de s'implémenter via des opérations binaires concises et rapides. Les opérations en lesquelles se réduisent
les instructions sont si simples qu'il semble possible de les compiler à la volée lors d'une première passe en
langage machine. On bénéficierait d'une exécution plus performante. Je n'ai pas implémenté ceci dans le rendu
actuel par manque de temps; cela reste un but pour le rendu final.

### Remarques
Quelques-un des test fournis ont été modifiés pour tester différents chemins d'exécution du simulateur.
Le dossier `test/testfiles` contient des fichiers d'entrée de test. Remarquons que les champs de 64 bits sont
stockés dans l'ordre petit-boutiste.

L'exemple `fulladder` est contre-intuitif avec les conventions d'entrée-sortie choisies; l'effet net est
que les entrées comme les sorties sont affichées avec l'ordre des bits inversé.
