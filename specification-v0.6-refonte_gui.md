# SealKey — Refonte complète de l’interface GUI pour la version 0.6

La conception actuelle de l’interface ne convient plus. Il faut remplacer l’interface fonctionnelle existante par une interface beaucoup plus directe, organisée autour de cinq onglets :

1. Crypter
2. Décrypter
3. Signer
4. Vérifier
5. Configuration

Cette nouvelle interface doit devenir la base de la version 0.6.

Ne pas conserver les anciens assistants ou écrans orientés « Envoyer », « Recevoir » ou « Préparer un dossier d’envoi », sauf si certains composants internes sont réutilisables sans apparaître dans la nouvelle interface.

L’objectif est d’obtenir une interface simple, prévisible et proche des opérations GPG classiques.

---

# 1. Onglet « Crypter »

## Sélection du fichier

Afficher :

* un label `Fichier`
* un champ texte affichant le chemin du fichier sélectionné
* un bouton `Choisir`

Le bouton `Choisir` ouvre un sélecteur de fichier.

Le champ peut être en lecture seule si cela simplifie l’interface, mais le chemin sélectionné doit rester visible intégralement ou être consultable.

## Sélection du destinataire

Afficher :

* un label `Destinataire`
* une liste des clés publiques disponibles

La liste doit permettre de sélectionner une seule clé publique de destinataire.

Chaque entrée doit être identifiable sans ambiguïté, par exemple avec :

* le nom ou l’identité principale ;
* l’adresse électronique lorsqu’elle existe ;
* l’identifiant court ou l’empreinte de la clé.

Ne pas afficher uniquement le nom si plusieurs clés peuvent avoir la même identité.

## Signature optionnelle

Afficher une case à cocher :

`Signer avec ma clé`

Lorsque cette case est cochée :

* le fichier doit être signé pendant l’opération de chiffrement ;
* la clé privée sélectionnée dans l’onglet `Configuration` doit être explicitement utilisée ;
* il ne faut pas laisser GPG choisir implicitement une autre clé privée par défaut.

Si aucune clé privée n’est sélectionnée dans la configuration :

* la case doit être désactivée ;
* ou une erreur claire doit être affichée avant l’opération.

## Bouton d’action

Afficher un bouton :

`Crypter`

Le bouton est actif uniquement lorsque :

* un fichier valide a été sélectionné ;
* un destinataire a été sélectionné.

Lorsque l’utilisateur clique sur `Crypter` :

* chiffrer le fichier pour la clé publique sélectionnée ;
* signer également le fichier si la case `Signer avec ma clé` est cochée ;
* utiliser explicitement la clé privée configurée pour la signature ;
* créer le fichier de sortie à côté du fichier source ;
* ajouter l’extension configurée dans l’onglet `Configuration`.

Exemple :

```text
document.pdf
document.pdf.gpg
```

Si le fichier de sortie existe déjà, demander confirmation avant de l’écraser.

Après réussite, afficher clairement :

* le chemin du fichier créé ;
* le destinataire utilisé ;
* si le fichier a également été signé.

---

# 2. Onglet « Décrypter »

## Sélection du fichier

Afficher :

* un label `Fichier`
* un champ texte affichant le chemin sélectionné
* un bouton `Choisir`

Le sélecteur doit filtrer prioritairement les fichiers possédant l’extension de fichier chiffré configurée.

Par exemple, si l’extension configurée est `gpg`, proposer les fichiers :

```text
*.gpg
```

Le filtrage ne doit pas empêcher complètement l’utilisateur d’afficher tous les fichiers, mais le format configuré doit être le choix par défaut.

## Liste des signatures

Afficher une zone ou une liste intitulée :

`Signataires`

Cette zone présente les signatures éventuellement contenues dans le fichier chiffré.

Pour chaque signature détectée, afficher au minimum :

* l’identité du signataire lorsqu’elle est connue ;
* l’identifiant ou l’empreinte de la clé ;
* l’état de la signature.

États possibles :

* `Signature valide`
* `Signature invalide`
* `Signataire inconnu`

`Signataire inconnu` signifie que la signature existe, mais que la clé publique nécessaire à son identification ou à sa vérification n’est pas présente dans le trousseau local.

Si le fichier n’est pas signé, afficher clairement :

`Aucune signature`

La liste doit être mise à jour après l’analyse ou le déchiffrement du fichier.

## Bouton d’action

Afficher un bouton :

`Décrypter`

Le bouton est actif uniquement lorsqu’un fichier valide a été sélectionné.

Lorsque l’utilisateur clique sur `Décrypter` :

* déchiffrer le fichier ;
* utiliser la clé privée adéquate du trousseau GPG ;
* vérifier automatiquement toutes les signatures intégrées ;
* enregistrer le fichier déchiffré à côté du fichier chiffré ;
* retirer l’extension de fichier chiffré configurée.

Exemple :

```text
document.pdf.gpg
document.pdf
```

Si le nom du fichier ne se termine pas par l’extension configurée, déterminer un nom de sortie raisonnable sans écraser le fichier source.

Si le fichier de sortie existe déjà, demander confirmation avant de l’écraser.

Après l’opération, afficher :

* le chemin du fichier déchiffré ;
* l’état général de l’opération ;
* le résultat de chaque signature.

Attention : le bouton `Décrypter` ne doit jamais ajouter l’extension de chiffrement. Il doit au contraire produire le fichier original ou retirer l’extension configurée.

---

# 3. Onglet « Signer »

## Sélection du fichier

Afficher :

* un label `Fichier`
* un champ texte affichant le chemin sélectionné
* un bouton `Choisir`

## Bouton d’action

Afficher un bouton :

`Signer`

Le bouton est actif uniquement lorsqu’un fichier valide a été sélectionné.

Lorsque l’utilisateur clique sur `Signer` :

* créer une signature détachée ;
* utiliser explicitement la clé privée sélectionnée dans l’onglet `Configuration` ;
* ne pas laisser GPG choisir implicitement une autre clé ;
* enregistrer la signature à côté du fichier d’origine ;
* utiliser l’extension de signature configurée.

Exemple :

```text
document.pdf
document.pdf.sig
```

Si aucune clé privée n’est configurée :

* désactiver le bouton ;
* ou afficher une erreur claire indiquant qu’une clé personnelle doit être sélectionnée dans l’onglet `Configuration`.

Si le fichier de signature existe déjà, demander confirmation avant de l’écraser.

Après réussite, afficher :

* le chemin de la signature créée ;
* l’identité de la clé utilisée ;
* l’empreinte ou l’identifiant de cette clé.

---

# 4. Onglet « Vérifier »

## Sélection du fichier

Afficher :

* un label `Fichier`
* un champ texte affichant le chemin du fichier original
* un bouton `Choisir`

## Sélection de la signature

Afficher :

* un label `Signature`
* un champ texte affichant le chemin de la signature
* un bouton `Choisir`

Le sélecteur de signature doit filtrer prioritairement les fichiers possédant l’extension de signature configurée.

Exemple :

```text
*.sig
```

Lorsque le fichier original est sélectionné, SealKey peut rechercher automatiquement, dans le même dossier, une signature portant le nom attendu.

Exemple :

```text
document.pdf
document.pdf.sig
```

Si cette signature existe, elle peut être sélectionnée automatiquement.

## Liste des signataires

Afficher une zone ou une liste intitulée :

`Signataires`

Pour chaque signature détectée, afficher :

* l’identité du signataire lorsqu’elle est connue ;
* l’identifiant ou l’empreinte de la clé ;
* le résultat de la vérification.

États possibles :

* `Signature valide`
* `Signature invalide`
* `Signataire inconnu`

Il ne faut pas employer les termes `récipients` ou `récipiendaires` dans cette interface. Le terme correct ici est `Signataires`.

La vérification doit être lancée lorsque les deux fichiers nécessaires sont sélectionnés, soit automatiquement, soit avec un bouton explicite `Vérifier`.

Il est préférable d’ajouter un bouton :

`Vérifier`

Ce bouton est actif lorsque :

* le fichier original est sélectionné ;
* le fichier de signature est sélectionné.

Après la vérification, afficher les résultats dans la liste des signataires.

---

# 5. Onglet « Configuration »

Toute modification valide effectuée dans cet onglet doit être enregistrée immédiatement dans les préférences.

Il ne doit pas être nécessaire de cliquer sur un bouton global `Enregistrer`.

Les préférences doivent être persistantes entre deux exécutions de SealKey.

## Chemin de l’exécutable GPG

Afficher :

* un label `Chemin de l’exécutable GPG`
* un champ texte
* un bouton `Choisir`
* un bouton `Détecter`
* un bouton `Tester`

### Bouton « Choisir »

Permet de sélectionner manuellement l’exécutable GPG.

### Bouton « Détecter »

Recherche automatiquement GPG dans les emplacements habituels de la plateforme.

Exemples possibles :

```text
gpg
/usr/bin/gpg
/usr/local/bin/gpg
/opt/local/bin/gpg
/opt/homebrew/bin/gpg
C:\Program Files\GnuPG\bin\gpg.exe
```

Ne pas coder uniquement ces chemins en dur. Utiliser aussi le `PATH` lorsque cela est pertinent.

### Bouton « Tester »

Exécute une commande non destructive telle que :

```text
gpg --version
```

Afficher clairement :

* si l’exécutable fonctionne ;
* la version détectée ;
* le chemin réellement utilisé ;
* le message d’erreur en cas d’échec.

Le chemin valide doit être enregistré dans les préférences.

---

## Ma clé

Afficher une liste des clés privées intitulée :

`Ma clé`

Cette liste sert à choisir la clé privée utilisée par défaut pour :

* signer un fichier ;
* signer pendant un chiffrement ;
* toute opération future nécessitant explicitement l’identité de l’utilisateur.

Chaque entrée doit afficher :

* l’identité principale ;
* l’adresse électronique lorsqu’elle existe ;
* l’identifiant de clé ;
* idéalement l’empreinte complète ;
* les capacités principales de la clé ;
* sa date d’expiration éventuelle.

La sélection doit être enregistrée immédiatement dans les préférences.

### Sélection automatique initiale

Lors du premier lancement :

* s’il n’existe qu’une seule clé privée utilisable, la sélectionner automatiquement ;
* s’il existe plusieurs clés privées, sélectionner de préférence celle présentant le meilleur niveau de confiance locale ;
* en cas d’égalité ou d’incertitude, ne pas choisir arbitrairement une clé potentiellement incorrecte : choisir une règle stable et documentée, ou laisser l’utilisateur choisir.

Une clé expirée, révoquée ou inutilisable pour signer ne doit pas être automatiquement privilégiée.

### Bouton « Créer une nouvelle clé privée »

Afficher un bouton :

`Créer une nouvelle clé privée`

Ce bouton ouvre une fenêtre modale.

La fenêtre doit permettre de saisir au minimum :

* le nom ;
* l’adresse électronique ;
* éventuellement un commentaire ;
* la durée de validité ;
* les paramètres supportés par l’application.

La génération doit utiliser GPG et afficher clairement les erreurs.

Après création :

* actualiser les listes de clés ;
* sélectionner si possible la nouvelle clé ;
* enregistrer cette sélection dans les préférences.

Ne jamais manipuler ou enregistrer la phrase secrète directement dans le fichier de préférences.

La saisie de la phrase secrète doit rester gérée par GPG ou `gpg-agent`.

### Bouton « Exporter la clé publique »

Afficher un bouton :

`Exporter ma clé publique`

Le bouton est actif uniquement lorsqu’une clé privée est sélectionnée.

Il doit exporter uniquement la partie publique de la clé sélectionnée.

Utiliser de préférence le format ASCII armor.

Exemple de commande conceptuelle :

```text
gpg --armor --export <empreinte>
```

Proposer un nom de fichier identifiable.

Exemple :

```text
Alexandre_Vialle_E63DDF8D_GPG_PUB.asc
```

---

## Destinataires

Afficher une liste de clés publiques intitulée :

`Destinataires`

Cette liste représente les clés publiques utilisables pour chiffrer des fichiers à destination d’autres personnes.

Éviter autant que possible d’y afficher la clé personnelle sélectionnée dans `Ma clé`, sauf si elle doit aussi être utilisée comme destinataire.

Chaque entrée doit afficher :

* l’identité principale ;
* l’adresse électronique lorsqu’elle existe ;
* l’identifiant de clé ;
* idéalement l’empreinte complète ;
* la date d’expiration éventuelle.

### Bouton « Ajouter la clé d’un destinataire »

Afficher un bouton :

`Ajouter la clé d’un destinataire`

Ce bouton permet de sélectionner un fichier contenant une clé publique, puis de l’importer avec GPG.

Formats potentiellement pris en charge :

```text
.asc
.gpg
.pgp
```

Après import :

* actualiser la liste des destinataires ;
* afficher le résultat de l’import ;
* signaler si la clé existait déjà ;
* afficher son identité et son empreinte.

### Bouton « Exporter la clé du destinataire »

Afficher un bouton :

`Exporter la clé du destinataire`

Le bouton est actif uniquement lorsqu’un destinataire est sélectionné.

Il exporte la clé publique du destinataire sélectionné, de préférence en ASCII armor.

---

## Extension du fichier chiffré

Afficher :

* un label `Extension du fichier chiffré`
* un champ texte

Valeur par défaut :

```text
gpg
```

Règles :

* supprimer les espaces placés avant et après la valeur ;
* accepter une saisie avec ou sans point initial ;
* enregistrer une représentation normalisée sans point initial ;
* si le champ devient vide après normalisation, restaurer `gpg` ;
* enregistrer immédiatement la valeur dans les préférences.

Exemples équivalents :

```text
gpg
.gpg
  gpg
```

Valeur enregistrée :

```text
gpg
```

L’interface doit ajouter elle-même le point lors de la création des noms de fichiers.

---

## Extension du fichier de signature

Afficher :

* un label `Extension du fichier de signature`
* un champ texte

Valeur par défaut :

```text
sig
```

Règles :

* supprimer les espaces placés avant et après la valeur ;
* accepter une saisie avec ou sans point initial ;
* enregistrer une représentation normalisée sans point initial ;
* si le champ devient vide après normalisation, restaurer `sig` ;
* enregistrer immédiatement la valeur dans les préférences.

---

# 6. Gestion des préférences

Conserver au minimum les préférences suivantes :

* position de la fenêtre ;
* taille de la fenêtre si elle est redimensionnable ;
* dernier onglet ouvert ;
* chemin de l’exécutable GPG ;
* empreinte complète de la clé privée sélectionnée ;
* extension du fichier chiffré ;
* extension du fichier de signature ;
* dernier dossier utilisé pour choisir un fichier ;
* éventuellement le dernier destinataire sélectionné.

Pour identifier les clés dans les préférences et les commandes GPG :

* utiliser de préférence l’empreinte complète ;
* ne pas se reposer uniquement sur un nom, une adresse électronique ou un identifiant court.

Les préférences doivent être enregistrées de manière sûre et atomique afin d’éviter un fichier corrompu si l’application est interrompue pendant l’écriture.

---

# 7. Exécution de GPG

Centraliser toutes les exécutions GPG dans un composant unique.

Ce composant doit gérer :

* la construction des arguments ;
* les chemins comportant des espaces ;
* la récupération de la sortie standard ;
* la récupération de la sortie d’erreur ;
* le code de retour ;
* l’annulation éventuelle ;
* les erreurs de lancement ;
* les résultats structurés de GPG.

Ne pas analyser uniquement les messages humains affichés sur la sortie standard ou la sortie d’erreur.

Utiliser autant que possible les interfaces structurées de GPG, notamment :

```text
--status-fd
--with-colons
--batch
```

Utiliser `--local-user` ou son équivalent lorsqu’une clé privée précise doit être utilisée pour signer.

Exemple conceptuel :

```text
gpg --local-user <empreinte-de-ma-cle> --detach-sign fichier
```

Pour chiffrer et signer :

```text
gpg --recipient <empreinte-destinataire> \
    --local-user <empreinte-de-ma-cle> \
    --sign \
    --encrypt \
    fichier
```

Les commandes exactes doivent être construites sous forme de liste d’arguments. Ne pas assembler une commande shell non échappée dans une chaîne unique.

---

# 8. États de l’interface

Les boutons doivent être activés ou désactivés selon l’état réel de l’interface.

## Onglet Crypter

`Crypter` actif si :

* un fichier existe ;
* un destinataire est sélectionné ;
* GPG est configuré et fonctionnel ;
* si la signature est demandée, une clé privée valide est sélectionnée.

## Onglet Décrypter

`Décrypter` actif si :

* un fichier existe ;
* GPG est configuré et fonctionnel.

## Onglet Signer

`Signer` actif si :

* un fichier existe ;
* une clé privée valide est sélectionnée ;
* GPG est configuré et fonctionnel.

## Onglet Vérifier

`Vérifier` actif si :

* le fichier original existe ;
* le fichier de signature existe ;
* GPG est configuré et fonctionnel.

## Onglet Configuration

Les boutons d’export doivent être désactivés lorsqu’aucune clé correspondante n’est sélectionnée.

---

# 9. Messages et erreurs

Ne pas afficher seulement une boîte de dialogue générique du type :

```text
Une erreur est survenue
```

Afficher un message compréhensible, comprenant selon le contexte :

* l’opération demandée ;
* le fichier concerné ;
* la clé concernée ;
* la cause probable ;
* le message retourné par GPG ;
* le code de retour.

Exemples :

```text
Le fichier n’a pas pu être chiffré pour Bob Dupont.
La clé sélectionnée est expirée.
```

```text
La signature est valide, mais la confiance accordée à cette clé n’est pas suffisante pour confirmer l’identité du signataire.
```

```text
La signature a été créée avec une clé publique absente du trousseau local.
Le signataire est inconnu.
```

Ne pas confondre :

* validité cryptographique d’une signature ;
* confiance accordée à la clé ;
* identité du propriétaire de la clé.

Une signature peut être cryptographiquement valide même si la clé n’est pas déclarée comme fiable.

---

# 10. Présentation graphique

Conserver FLTK et la compilation statique prévue par les spécifications existantes.

L’interface doit rester compacte et native.

Éviter :

* les assistants en plusieurs étapes ;
* les écrans trop chargés ;
* les grandes zones décoratives ;
* les icônes indispensables à la compréhension ;
* les contrôles sans label textuel ;
* les boîtes de dialogue inutiles.

Les cinq onglets doivent être visibles dans la fenêtre principale.

Adopter un alignement régulier :

* labels dans une colonne ;
* champs dans une autre ;
* boutons associés à droite ;
* boutons d’action principaux clairement séparés.

Prévoir une taille minimale de fenêtre permettant d’afficher les empreintes ou les identités sans rendre les listes inutilisables.

---

# 11. Nettoyage du code existant

Supprimer ou désactiver :

* les anciens écrans qui ne correspondent plus à cette conception ;
* les anciens intitulés ;
* les comportements qui choisissent implicitement une clé privée ;
* les doublons entre l’ancienne interface et la nouvelle ;
* les fonctionnalités devenues inaccessibles depuis la GUI.

Conserver et réutiliser les composants internes pertinents :

* lecture des clés ;
* import et export ;
* exécution de GPG ;
* préférences ;
* détection de GPG ;
* gestion des fichiers.

Refactoriser si nécessaire afin que la logique métier ne soit pas directement implantée dans les callbacks FLTK.

---

# 12. Tests attendus

Ajouter ou mettre à jour les tests pour couvrir au minimum :

## Préférences

* valeur par défaut `gpg` ;
* valeur par défaut `sig` ;
* suppression des espaces ;
* suppression du point initial ;
* restauration de la valeur par défaut si le champ est vide ;
* persistance de la clé privée par empreinte ;
* persistance du chemin GPG.

## Activation des boutons

* activation et désactivation de `Crypter` ;
* activation et désactivation de `Décrypter` ;
* activation et désactivation de `Signer` ;
* activation et désactivation de `Vérifier` ;
* activation et désactivation des boutons d’export.

## Nommage des fichiers

* ajout de `.gpg` ;
* retrait de `.gpg` pendant le déchiffrement ;
* ajout de `.sig` ;
* comportement avec une extension personnalisée ;
* comportement lorsqu’un fichier cible existe déjà.

## Clés

* une seule clé privée disponible ;
* plusieurs clés privées ;
* clé expirée ;
* clé révoquée ;
* clé publique inconnue ;
* sélection explicite de la clé avec laquelle signer.

## Signatures

* signature valide ;
* signature invalide ;
* signature valide avec clé non fiable ;
* signataire inconnu ;
* fichier non signé ;
* plusieurs signatures si GPG en retourne plusieurs.

## GPG

* exécutable valide ;
* exécutable absent ;
* exécutable non lançable ;
* version détectée ;
* erreur retournée par GPG.

---

# 13. Critères d’acceptation de la version 0.6

La version 0.6 est considérée comme conforme lorsque :

1. La fenêtre principale contient exactement les cinq onglets demandés.
2. Un fichier peut être chiffré pour une clé publique sélectionnée.
3. Le chiffrement peut être signé avec la clé privée configurée.
4. La clé privée de signature est toujours explicitement transmise à GPG.
5. Un fichier chiffré peut être déchiffré.
6. Les signatures intégrées sont vérifiées et affichées.
7. Une signature détachée peut être créée.
8. Une signature détachée peut être vérifiée.
9. Les clés publiques peuvent être importées et exportées.
10. La clé publique correspondant à la clé privée personnelle peut être exportée.
11. Le chemin de GPG peut être choisi, détecté et testé.
12. Les extensions sont configurables et immédiatement persistées.
13. Les préférences survivent au redémarrage de l’application.
14. Aucun ancien écran incompatible avec cette conception n’est encore exposé.
15. Le projet compile sur les plateformes déjà prévues par les spécifications.
16. Les tests existants restent fonctionnels ou sont adaptés à la nouvelle architecture.
17. Le numéro de version de l’application est mis à jour en `0.6`.

Avant de terminer, compiler le projet, exécuter les tests et corriger les erreurs ou avertissements significatifs.

Fournir ensuite un compte rendu indiquant :

* les fichiers modifiés ;
* les anciens composants supprimés ;
* les nouveaux composants ajoutés ;
* les commandes GPG employées ;
* les tests ajoutés ;
* les éventuelles limitations restantes.

