# Évolution ergonomique de SealKey

## 1. Objectif ergonomique

SealKey doit être pensé comme un assistant graphique simple pour les usages courants de GPG.

L’utilisateur ne doit pas avoir à raisonner d’abord en termes de commandes `gpg --encrypt`, `gpg --sign`, `gpg --verify`, `--armor`, `--detach-sign`, etc.

L’interface doit partir de cas d’usage humains :

* envoyer un fichier à quelqu’un ;
* recevoir un fichier sécurisé ;
* signer un document ;
* vérifier l’origine d’un document ;
* gérer ses clés ;
* exporter sa clé publique ;
* produire un ensemble de fichiers prêts à envoyer par email.

SealKey doit donc proposer une interface centrée sur des scénarios.

---

# 2. Personnages de référence

## Alice

Alice veut envoyer des documents confidentiels ou signés à Bob.

Elle possède :

* une clé privée GPG ;
* une clé publique associée ;
* éventuellement plusieurs identités ;
* une liste de clés publiques de correspondants.

Elle veut pouvoir :

* chiffrer un fichier pour Bob ;
* signer un fichier avec sa clé ;
* chiffrer et signer en une seule opération ;
* joindre facilement sa clé publique ;
* vérifier que l’opération a réussi.

## Bob

Bob reçoit un fichier d’Alice.

Il veut pouvoir :

* déchiffrer le fichier s’il lui est destiné ;
* vérifier la signature d’Alice ;
* importer la clé publique d’Alice si nécessaire ;
* comprendre clairement si le fichier est fiable ou non ;
* obtenir un message compréhensible en cas d’échec.

---

# 3. Cas d’usage principaux

## UC-01 — Chiffrer un fichier pour un destinataire

### Acteur principal

Alice

### Objectif

Alice veut envoyer un fichier confidentiel à Bob.

### Préconditions

* GPG est disponible.
* Alice possède la clé publique de Bob.
* Le fichier source existe.

### Scénario nominal

1. Alice ouvre SealKey.

2. Elle choisit l’action : **Envoyer un fichier confidentiel**.

3. Elle sélectionne le fichier à protéger.

4. SealKey lui demande : **À qui voulez-vous l’envoyer ?**

5. Alice choisit Bob dans la liste des clés publiques disponibles.

6. SealKey propose un nom de fichier de sortie, par exemple :
   
   `document.pdf.gpg`

7. Alice valide.

8. SealKey chiffre le fichier avec la clé publique de Bob.

9. SealKey affiche un résultat clair :
   
   “Le fichier a été chiffré pour Bob. Seul Bob pourra le déchiffrer avec sa clé privée.”

### Variante

Alice peut choisir plusieurs destinataires.

Dans ce cas, le fichier est chiffré pour plusieurs clés publiques.

### Points ergonomiques importants

* Ne pas afficher “recipient” comme terme principal.
* Préférer : “Destinataire”.
* Afficher l’identité lisible de la clé : nom, email, empreinte courte.
* Avertir si la confiance de la clé est inconnue, sans bloquer l’opération.

---

## UC-02 — Signer un fichier

### Acteur principal

Alice

### Objectif

Alice veut prouver qu’un fichier vient bien d’elle et qu’il n’a pas été modifié.

### Préconditions

* Alice possède une clé privée permettant de signer.
* Le fichier source existe.

### Scénario nominal

1. Alice choisit l’action : **Signer un fichier**.

2. Elle sélectionne le fichier.

3. SealKey lui propose la clé de signature à utiliser.

4. SealKey propose par défaut une signature détachée :
   
   `document.pdf.sig`

5. Alice valide.

6. SealKey génère la signature.

7. SealKey affiche :
   
   “Signature créée. Envoyez le fichier original et le fichier de signature au destinataire.”

### Variante

Alice peut choisir une signature ASCII armor :

`document.pdf.asc`

### Points ergonomiques importants

* Expliquer la différence entre :
  
  * signature détachée ;
  * fichier signé complet ;
  * signature ASCII.

* Par défaut, privilégier la signature détachée, plus compatible avec l’envoi de documents existants.

---

## UC-03 — Chiffrer et signer un fichier

### Acteur principal

Alice

### Objectif

Alice veut envoyer à Bob un fichier confidentiel, tout en prouvant qu’il vient d’elle.

### Préconditions

* Alice possède une clé privée de signature.
* Alice possède la clé publique de Bob.
* Le fichier source existe.

### Scénario nominal

1. Alice choisit l’action : **Envoyer un fichier confidentiel et signé**.

2. Elle sélectionne le fichier.

3. Elle choisit Bob comme destinataire.

4. Elle choisit sa clé de signature, ou SealKey utilise la dernière clé choisie.

5. SealKey propose :
   
   `document.pdf.gpg`

6. Alice valide.

7. SealKey chiffre le fichier pour Bob et le signe avec la clé d’Alice.

8. SealKey affiche :
   
   “Le fichier est chiffré pour Bob et signé par Alice.”

### Points ergonomiques importants

* C’est probablement le scénario le plus utile.

* Il doit être accessible directement depuis l’écran principal.

* Le libellé doit éviter le jargon.

* Exemple de bouton principal :
  
  **Protéger un fichier pour quelqu’un**

---

## UC-04 — Déchiffrer un fichier reçu

### Acteur principal

Bob

### Objectif

Bob reçoit un fichier `.gpg` et veut l’ouvrir.

### Préconditions

* Bob possède la clé privée correspondant au chiffrement.
* GPG est disponible.

### Scénario nominal

1. Bob ouvre SealKey.

2. Il choisit l’action : **Ouvrir un fichier protégé**.

3. Il sélectionne le fichier `.gpg`.

4. SealKey tente le déchiffrement.

5. Si une passphrase est nécessaire, GPG ou l’agent GPG la demande.

6. SealKey produit le fichier déchiffré.

7. SealKey affiche :
   
   “Le fichier a été déchiffré avec succès.”

### Variante : fichier signé

Si le fichier contient aussi une signature, SealKey affiche :

* signature valide ;
* signature invalide ;
* clé inconnue ;
* impossible de vérifier.

### Points ergonomiques importants

* Ne jamais afficher seulement le retour brut de GPG.

* Traduire les erreurs fréquentes.

* Exemple :
  
  “Aucune clé privée compatible n’a été trouvée. Ce fichier n’a probablement pas été chiffré pour vous.”

---

## UC-05 — Vérifier une signature

### Acteur principal

Bob

### Objectif

Bob veut vérifier qu’un fichier reçu vient bien d’Alice et n’a pas été modifié.

### Préconditions

* Bob possède le fichier original.
* Bob possède le fichier de signature.
* Bob possède éventuellement la clé publique d’Alice.

### Scénario nominal

1. Bob choisit l’action : **Vérifier une signature**.
2. Il sélectionne le fichier original.
3. Il sélectionne le fichier de signature.
4. SealKey lance la vérification.
5. SealKey affiche un résultat clair.

### Résultats possibles

#### Signature valide

“Signature valide. Le fichier correspond bien à la signature d’Alice.”

#### Signature invalide

“Signature invalide. Le fichier ne correspond pas à cette signature, ou il a été modifié.”

#### Clé publique manquante

“Impossible de vérifier la signature : la clé publique de l’expéditeur n’est pas connue.”

SealKey propose alors :

* importer une clé publique ;
* sélectionner un fichier de clé publique ;
* réessayer la vérification.

### Points ergonomiques importants

L’écran de résultat doit être très explicite.

Il faut éviter les messages ambigus comme :

* “Good signature from unknown trust”
* “Can’t check signature: No public key”

Il faut les traduire en langage utilisateur.

---

## UC-06 — Importer une clé publique

### Acteur principal

Bob

### Objectif

Bob veut importer la clé publique d’Alice pour vérifier ses signatures ou lui envoyer des fichiers.

### Préconditions

* Bob possède un fichier de clé publique, par exemple `.asc`, `.gpg`, `.txt`.

### Scénario nominal

1. Bob choisit : **Importer une clé publique**.

2. Il sélectionne le fichier de clé.

3. SealKey importe la clé.

4. SealKey affiche :
   
   * nom ;
   * email ;
   * empreinte ;
   * date de création ;
   * capacités de la clé : signature, chiffrement.

5. Bob valide l’import.

### Points ergonomiques importants

Après import, SealKey doit proposer une action suivante :

* “Vérifier maintenant un fichier signé avec cette clé”
* “Utiliser cette clé pour chiffrer un fichier”
* “Voir les détails de la clé”

---

## UC-07 — Exporter sa clé publique

### Acteur principal

Alice

### Objectif

Alice veut envoyer sa clé publique à Bob.

### Préconditions

* Alice possède une clé GPG.

### Scénario nominal

1. Alice choisit : **Exporter ma clé publique**.

2. SealKey propose la clé par défaut.

3. Alice peut choisir une autre clé.

4. SealKey propose un nom de fichier lisible :
   
   `Alice_DUPONT_1234ABCD_GPG_PUB.asc`

5. Alice valide.

6. SealKey exporte la clé publique.

7. SealKey affiche :
   
   “Votre clé publique a été exportée. Vous pouvez l’envoyer à vos correspondants.”

### Points ergonomiques importants

* Le nom du fichier doit être explicite.
* L’empreinte courte doit apparaître dans le nom.
* L’utilisateur doit comprendre que ce fichier peut être envoyé publiquement.

---

## UC-08 — Préparer un paquet complet à envoyer

### Acteur principal

Alice

### Objectif

Alice veut envoyer plusieurs éléments cohérents à Bob :

* le document ;
* la signature ;
* sa clé publique ;
* éventuellement une courte note explicative.

### Scénario nominal

1. Alice choisit : **Préparer un envoi complet**.

2. Elle sélectionne un ou plusieurs fichiers.

3. Elle choisit :
   
   * signer seulement ;
   * chiffrer seulement ;
   * chiffrer et signer.

4. Elle choisit le destinataire.

5. SealKey génère les fichiers nécessaires.

6. SealKey crée un dossier de sortie :
   
   `SealKey_export_2026-07-02/`

7. Le dossier contient par exemple :
   
   * `document.pdf.gpg`
   * `Alice_DUPONT_1234ABCD_GPG_PUB.asc`
   * `README.txt`

8. SealKey affiche :
   
   “Dossier prêt à envoyer.”

### Exemple de README généré

```text
Ce dossier contient un fichier protégé avec GPG.

Fichier :
document.pdf.gpg

Ce fichier a été chiffré pour :
Bob Martin <bob@example.net>

Il a été signé par :
Alice Dupont <alice@example.net>

La clé publique d’Alice est jointe pour permettre la vérification de la signature.
```

### Points ergonomiques importants

Ce cas d’usage est très intéressant pour SealKey.

Il transforme SealKey en outil pratique d’envoi, pas seulement en interface GPG.

---

# 4. Scénarios Alice / Bob

## Scénario 1 — Alice envoie un document confidentiel à Bob

Alice doit envoyer à Bob un contrat confidentiel.

Elle ouvre SealKey et choisit :

**Protéger un fichier pour quelqu’un**

SealKey lui demande :

1. Quel fichier voulez-vous protéger ?
2. À qui voulez-vous l’envoyer ?
3. Voulez-vous aussi signer le fichier ?

Alice sélectionne :

* `contrat.pdf`
* Bob comme destinataire
* signature activée

SealKey produit :

`contrat.pdf.gpg`

Alice envoie ce fichier à Bob par email.

Bob ouvre SealKey et choisit :

**Ouvrir un fichier protégé**

Il sélectionne :

`contrat.pdf.gpg`

SealKey déchiffre le fichier et affiche :

“Fichier déchiffré avec succès. Signature valide d’Alice Dupont.”

Bob sait alors :

* que le fichier lui était destiné ;
* qu’Alice l’a bien signé ;
* que le contenu n’a pas été modifié.

---

## Scénario 2 — Alice envoie des documents administratifs signés

Alice doit envoyer plusieurs documents PDF à un interlocuteur administratif.

Elle ne veut pas forcément chiffrer les documents, mais elle veut prouver qu’ils viennent d’elle.

Elle choisit :

**Signer des fichiers**

Elle sélectionne plusieurs PDF.

SealKey génère :

* `document1.pdf.sig`
* `document2.pdf.sig`
* `document3.pdf.sig`
* `Alexandre_VIALLE_E63DDF8D_GPG_PUB.asc`

SealKey propose de créer un dossier :

`documents_signes_2026-07-02/`

Avec un README expliquant :

* que les fichiers `.sig` sont les signatures ;
* que la clé publique permet de vérifier les documents ;
* que les fichiers originaux doivent rester inchangés.

L’utilisateur peut ensuite envoyer le dossier complet.

---

## Scénario 3 — Bob reçoit une signature mais pas la clé publique

Bob reçoit :

* `document.pdf`
* `document.pdf.sig`

Il ouvre SealKey et choisit :

**Vérifier une signature**

SealKey répond :

“Impossible de vérifier la signature : la clé publique de l’expéditeur n’est pas connue.”

SealKey propose :

* importer une clé publique ;
* demander la clé publique à l’expéditeur ;
* annuler.

Bob importe ensuite :

`Alice_DUPONT_1234ABCD_GPG_PUB.asc`

SealKey relance automatiquement la vérification.

Résultat :

“Signature valide. Le fichier correspond à la signature d’Alice Dupont.”

---

## Scénario 4 — Bob reçoit un fichier qui ne lui était pas destiné

Bob reçoit :

`archive.zip.gpg`

Il tente de l’ouvrir avec SealKey.

GPG échoue.

SealKey affiche :

“Ce fichier ne peut pas être déchiffré avec vos clés privées. Il n’a probablement pas été chiffré pour vous, ou vous n’utilisez pas le bon trousseau GPG.”

SealKey propose :

* afficher les détails techniques ;
* choisir un autre exécutable GPG ;
* vérifier les clés disponibles ;
* fermer.

---

## Scénario 5 — Alice se trompe de destinataire

Alice veut envoyer un fichier à Bob, mais choisit par erreur une clé nommée Bob sans vérifier l’email.

SealKey affiche avant validation :

```text
Vous allez chiffrer ce fichier pour :

Bob Martin <bob@example.net>
Empreinte : A1B2 C3D4 E5F6 7890

Attention : la confiance de cette clé n’est pas établie.
```

Alice peut :

* confirmer ;
* choisir une autre clé ;
* voir les détails de la clé.

Cela évite les erreurs silencieuses.

---

# 5. Évolution de l’interface

## 5.1 Écran d’accueil

L’écran d’accueil doit présenter des actions simples.

### Actions principales

* **Protéger un fichier pour quelqu’un**
* **Ouvrir un fichier protégé**
* **Signer un fichier**
* **Vérifier une signature**
* **Importer une clé publique**
* **Exporter ma clé publique**

### Actions secondaires

* Paramètres
* Voir les clés disponibles
* Tester la configuration GPG
* Afficher le journal technique

---

## 5.2 Assistant guidé

Certaines opérations doivent utiliser un assistant en plusieurs étapes.

Exemple pour “Protéger un fichier pour quelqu’un” :

### Étape 1 — Fichier

* Sélection du fichier source
* Affichage du nom, taille, chemin

### Étape 2 — Destinataire

* Liste des clés publiques capables de chiffrer
* Recherche par nom ou email
* Affichage de l’empreinte courte
* Indicateur de confiance

### Étape 3 — Signature

* Case à cocher : “Signer aussi le fichier”
* Sélection de la clé de signature
* Option : “Utiliser cette clé par défaut”

### Étape 4 — Sortie

* Nom du fichier généré
* Dossier de destination
* Option ASCII armor
* Résumé avant exécution

### Étape 5 — Résultat

* Succès ou échec
* Chemin du fichier produit
* Bouton “Ouvrir le dossier”
* Bouton “Copier le chemin”
* Bouton “Afficher les détails techniques”

---

## 5.3 Résultats compréhensibles

SealKey doit distinguer deux niveaux d’information :

### Niveau utilisateur

Message clair, court, compréhensible.

Exemple :

“Signature valide. Le fichier n’a pas été modifié depuis sa signature par Alice.”

### Niveau technique

Sortie GPG brute ou semi-brute, accessible via :

**Afficher les détails techniques**

Cela permet de rester simple pour les utilisateurs normaux, sans cacher l’information utile aux utilisateurs avancés.

---

# 6. Améliorations fonctionnelles proposées

## 6.1 Mode “Simple” et mode “Avancé”

SealKey peut proposer deux niveaux d’interface.

### Mode Simple

Affiche uniquement les actions principales et les options sûres.

### Mode Avancé

Expose davantage d’options :

* ASCII armor ;
* signature détachée ou intégrée ;
* compression ;
* choix précis du trust model ;
* choix d’un homedir GPG ;
* options supplémentaires passées à GPG ;
* affichage complet des empreintes ;
* export/import avancé.

Par défaut, SealKey doit rester en mode Simple.

---

## 6.2 Détection automatique du type de fichier

Quand l’utilisateur ouvre un fichier, SealKey peut détecter l’action probable :

* `.gpg` → déchiffrer
* `.asc` → importer une clé ou vérifier une signature ASCII
* `.sig` → vérifier une signature
* `.pgp` → déchiffrer ou importer selon le contenu

Si l’utilisateur glisse-dépose un fichier dans SealKey, l’application propose l’action la plus probable.

---

## 6.3 Glisser-déposer

L’utilisateur doit pouvoir glisser un fichier sur la fenêtre principale.

SealKey affiche alors :

“Que voulez-vous faire avec ce fichier ?”

Actions possibles selon le fichier :

* chiffrer ;
* signer ;
* vérifier ;
* déchiffrer ;
* importer une clé.

---

## 6.4 Préférences persistantes

SealKey doit mémoriser :

* dernier chemin utilisé ;
* dernière position et taille de fenêtre ;
* dernier exécutable GPG sélectionné ;
* dernière clé de signature ;
* dernière clé de chiffrement personnelle ;
* dernier dossier de sortie ;
* mode simple ou avancé ;
* préférence ASCII armor ;
* préférence de signature détachée ;
* choix “ouvrir le dossier après génération”.

---

## 6.5 Carnet de correspondants

SealKey peut introduire une notion simple de “correspondants”.

Un correspondant est une clé publique présentée de manière humaine.

Exemple :

```text
Bob Martin
bob@example.net
Clé : A1B2 C3D4
Peut recevoir des fichiers chiffrés
Confiance : inconnue
```

Cela évite de faire manipuler directement des clés GPG brutes.

Fonctions possibles :

* voir les correspondants ;
* importer une clé publique ;
* renommer un correspondant localement ;
* voir l’empreinte complète ;
* supprimer une clé publique ;
* exporter une clé publique ;
* vérifier les capacités de la clé.

---

## 6.6 Assistant de diagnostic GPG

SealKey doit proposer un test de configuration.

L’action **Tester GPG** vérifie :

* présence de l’exécutable GPG ;
* version ;
* accès au trousseau ;
* liste des clés publiques ;
* liste des clés privées ;
* capacité à signer ;
* capacité à chiffrer ;
* présence de `gpg-agent`.

Résultat attendu :

```text
GPG est correctement configuré.

Version : 2.4.x
Clés privées disponibles : 1
Clés publiques disponibles : 7
Agent GPG : actif
```

En cas d’erreur :

```text
GPG a été trouvé, mais aucune clé privée n’est disponible.
Vous pourrez importer des clés publiques, mais vous ne pourrez pas signer ni déchiffrer.
```

---

# 7. Messages d’erreur ergonomiques

## Erreur : aucune clé publique pour chiffrer

Message utilisateur :

“Aucune clé publique de destinataire n’est disponible. Pour chiffrer un fichier à quelqu’un, vous devez d’abord importer sa clé publique.”

Actions proposées :

* Importer une clé publique
* Annuler

## Erreur : aucune clé privée pour signer

Message utilisateur :

“Aucune clé privée de signature n’est disponible. SealKey ne peut pas signer de fichier avec cette configuration GPG.”

Actions proposées :

* Tester GPG
* Choisir un autre exécutable GPG
* Voir les détails techniques

## Erreur : mauvaise passphrase

Message utilisateur :

“La phrase de passe a été refusée ou l’opération a été annulée.”

Actions proposées :

* Réessayer
* Annuler

## Erreur : signature invalide

Message utilisateur :

“Signature invalide. Le fichier ne correspond pas à cette signature. Il a peut-être été modifié, ou la signature ne correspond pas à ce fichier.”

Actions proposées :

* Choisir un autre fichier original
* Choisir une autre signature
* Voir les détails techniques

---

# 8. Recommandations d’ergonomie

## 8.1 Toujours montrer un résumé avant exécution

Avant une opération sensible, SealKey doit afficher :

```text
Résumé

Fichier source :
/Users/alice/Documents/contrat.pdf

Action :
Chiffrer et signer

Destinataire :
Bob Martin <bob@example.net>

Signature :
Alice Dupont <alice@example.net>

Fichier de sortie :
/Users/alice/Documents/contrat.pdf.gpg
```

Boutons :

* Annuler
* Retour
* Exécuter

---

## 8.2 Toujours montrer ce qui a été produit

Après une opération, SealKey doit afficher clairement :

* le fichier créé ;
* son emplacement ;
* l’action effectuée ;
* les éventuelles limites.

Exemple :

```text
Opération terminée

Fichier créé :
contrat.pdf.gpg

Ce fichier est chiffré pour :
Bob Martin <bob@example.net>

Ce fichier est signé par :
Alice Dupont <alice@example.net>
```

Boutons :

* Ouvrir le dossier
* Copier le chemin
* Nouvelle opération

---

## 8.3 Ne jamais masquer les détails techniques

Les détails GPG doivent rester disponibles.

Mais ils ne doivent pas être l’affichage principal.

Structure recommandée :

* message utilisateur clair ;
* bouton “Détails techniques” ;
* zone repliable avec sortie GPG.

---

# 9. Évolution proposée de la structure applicative

## 9.1 Modèle conceptuel

Ajouter des objets métier simples :

### GpgIdentity

Représente une identité locale disposant d’une clé privée.

Champs :

* name
* email
* fingerprint
* key_id
* can_sign
* can_decrypt
* created_at
* expires_at

### GpgContact

Représente un correspondant disposant d’une clé publique.

Champs :

* name
* email
* fingerprint
* key_id
* can_encrypt
* can_verify
* trust_level
* created_at
* expires_at

### SealKeyOperation

Représente une opération utilisateur.

Types :

* encrypt
* decrypt
* sign
* verify
* encrypt_and_sign
* import_public_key
* export_public_key
* prepare_package

### OperationResult

Résultat d’une opération.

Champs :

* success
* user_message
* technical_message
* output_files
* warnings
* suggested_next_actions

---

## 9.2 Séparation logique

L’application devrait être organisée en couches :

### UI

* fenêtres FLTK ;
* assistants ;
* formulaires ;
* messages utilisateur.

### Application

* cas d’usage ;
* validation ;
* préférences ;
* orchestration des opérations.

### GPG Backend

* construction des commandes ;
* exécution ;
* parsing des sorties ;
* conversion des erreurs en messages compréhensibles.

### Storage

* préférences ;
* historique local optionnel ;
* derniers choix.

---

# 10. Priorités de développement

## Priorité 1 — Ergonomie minimale solide

* écran d’accueil par actions ;
* chiffrer ;
* signer ;
* chiffrer et signer ;
* déchiffrer ;
* vérifier ;
* importer une clé publique ;
* exporter sa clé publique ;
* préférences persistantes ;
* messages d’erreur lisibles.

## Priorité 2 — Assistant guidé

* étapes claires ;
* résumé avant exécution ;
* résultat structuré ;
* détails techniques repliables.

## Priorité 3 — Paquet d’envoi

* génération d’un dossier complet ;
* inclusion optionnelle de la clé publique ;
* README généré ;
* signatures détachées multiples.

## Priorité 4 — Carnet de correspondants

* affichage simplifié des clés publiques ;
* recherche ;
* détails ;
* capacités de clé ;
* confiance.

## Priorité 5 — Mode avancé

* options GPG avancées ;
* armor ;
* homedir ;
* trust model ;
* affichage complet des empreintes ;
* options supplémentaires.

---

# 11. Proposition de nouvel écran principal

L’écran principal pourrait être organisé ainsi :

```text
SealKey
Lightweight GPG desktop helper

Que voulez-vous faire ?

[ Protéger un fichier pour quelqu’un ]
[ Ouvrir un fichier protégé ]

[ Signer un fichier ]
[ Vérifier une signature ]

[ Importer une clé publique ]
[ Exporter ma clé publique ]

-------------------------------------

Configuration GPG : OK
Clé de signature par défaut : Alexandre Vialle <...>
Dernier dossier utilisé : ~/Documents

[ Paramètres ] [ Tester GPG ] [ Détails ]
```

---

# 12. Intention produit

SealKey doit se positionner comme un petit outil graphique de confiance pour les opérations GPG courantes.

Il ne doit pas chercher à remplacer GPG, Kleopatra ou GPA.

Son intérêt est ailleurs :

* interface très simple ;
* peu de dépendances ;
* application légère ;
* fonctionnement local ;
* messages compréhensibles ;
* scénarios concrets ;
* export facile de fichiers prêts à envoyer.

SealKey doit être un “assistant de scellement numérique” : l’utilisateur choisit une intention, SealKey traduit cette intention en opération GPG correcte.
