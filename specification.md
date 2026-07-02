# Spécification complète — SealKey

## 1. Nom du projet

Le projet s’appelle :

```text
SealKey
```

Nom complet affiché dans l’application :

```text
SealKey — Lightweight GPG desktop helper
```

Noms techniques à utiliser :

```text
Nom affiché : SealKey
Nom complet : SealKey — Lightweight GPG desktop helper
Nom du projet : sealkey
Nom du dépôt : sealkey
Nom de l’exécutable : sealkey
Dossier de configuration : sealkey
Fichier de préférences : preferences.ini
```

Chemins de configuration à utiliser :

```text
Linux  : ~/.config/sealkey/preferences.ini
macOS  : ~/Library/Application Support/sealkey/preferences.ini
Windows: %APPDATA%\sealkey\preferences.ini
```

Le code doit centraliser ces constantes dans un seul endroit, par exemple :

```cpp
namespace AppInfo {
    constexpr const char* Name = "SealKey";
    constexpr const char* FullName = "SealKey — Lightweight GPG desktop helper";
    constexpr const char* ProjectId = "sealkey";
    constexpr const char* PreferencesFile = "preferences.ini";
}
```

---

## 2. Objectif général

Développer une petite application graphique multiplateforme permettant d’utiliser GPG simplement, sans réimplémenter OpenPGP.

L’application doit permettre de :

1. sélectionner l’exécutable `gpg` ;
2. tester l’exécutable `gpg` ;
3. lister les clés disponibles ;
4. sélectionner une clé de chiffrement ;
5. sélectionner une clé de signature ;
6. exporter la clé publique sélectionnée ;
7. chiffrer et déchiffrer un fichier ;
8. chiffrer et déchiffrer un texte saisi ou collé ;
9. signer et vérifier la signature d’un fichier ;
10. signer et vérifier la signature d’un texte saisi ou collé ;
11. mémoriser les préférences utilisateur non sensibles.

L’application doit fonctionner sous :

* Windows x86_64 ;
* Windows ARM64 si possible ;
* Linux x86_64 ;
* Linux ARM64 ;
* macOS Intel ;
* macOS Apple Silicon.

---

## 3. Choix technologiques

Le projet doit être développé avec :

```text
C++20
CMake
FLTK
```

Le toolkit graphique retenu est **FLTK**.

L’objectif est de produire une application :

```text
petite
portable
simple
compilable facilement
avec dépendances minimales
```

L’application doit privilégier la **liaison statique de FLTK** lorsque cela est raisonnablement possible.

L’application ne doit pas utiliser :

* Qt ;
* wxWidgets ;
* GTK ;
* GTKmm ;
* Electron ;
* libgpgme ;
* OpenSSL ;
* bibliothèque cryptographique tierce ;
* dépendances graphiques lourdes ou exotiques.

L’application doit appeler l’exécutable `gpg` installé localement.

Elle ne doit pas embarquer ni réimplémenter de moteur cryptographique.

---

## 4. Principe général

L’application est une interface graphique autour de GnuPG.

Elle délègue toutes les opérations cryptographiques à `gpg`.

Elle ne doit jamais :

* manipuler directement les clés privées ;
* stocker de passphrase ;
* demander elle-même une passphrase ;
* stocker de texte clair ;
* stocker de texte chiffré ;
* stocker de signature ;
* modifier le trousseau GPG sans action explicite.

Les demandes de passphrase doivent rester gérées par :

```text
gpg-agent
pinentry
mécanismes standards de GnuPG
```

---

## 5. Contraintes de dépendances

Dépendances autorisées :

* bibliothèque standard C++ ;
* FLTK ;
* dépendances système strictement nécessaires à FLTK ;
* CMake.

Dépendances à éviter :

* framework graphique lourd ;
* bibliothèque cryptographique tierce ;
* bibliothèque GPG externe comme `libgpgme` ;
* dépendance réseau ;
* dépendance JSON lourde uniquement pour les préférences.

Pour la configuration locale, utiliser de préférence un fichier INI simple, avec un parser minimal interne.

---

## 6. Liaison statique

### 6.1 Objectif

L’objectif est :

```text
FLTK statique
exécutable aussi autonome que possible
pas de DLL FLTK à fournir séparément
pas de framework graphique lourd
```

### 6.2 FLTK

FLTK doit pouvoir être utilisé selon deux modes :

```text
USE_SYSTEM_FLTK=ON
USE_SYSTEM_FLTK=OFF
```

Mode recommandé pour la distribution :

```text
USE_SYSTEM_FLTK=OFF
```

Dans ce mode, FLTK peut être :

* inclus comme sous-module Git ;
* placé dans `third_party/fltk` ;
* téléchargé par CMake via `FetchContent`.

L’objectif est que l’utilisateur final n’ait pas à installer FLTK séparément.

---

### 6.3 Windows

Sous Windows, l’objectif est de produire un `.exe` ne dépendant pas de DLL FLTK externes.

Si MinGW est utilisé, prévoir si possible :

```text
-static
-static-libgcc
-static-libstdc++
```

Si MSVC est utilisé, documenter clairement le choix du runtime :

* `/MT` pour runtime statique ;
* `/MD` pour runtime dynamique.

L’objectif prioritaire est :

```text
pas de dépendance FLTK externe à côté de l’exécutable
```

---

### 6.4 macOS

Sous macOS, l’objectif est de produire une application `.app` simple.

FLTK doit être lié statiquement si possible.

L’application ne doit pas embarquer `gpg`.

Elle doit permettre à l’utilisateur de choisir explicitement le chemin de `gpg`.

Chemins typiques à tester :

```text
/opt/local/bin/gpg
/opt/homebrew/bin/gpg
/usr/local/bin/gpg
/usr/bin/gpg
```

`/usr/bin/gpg` peut ne pas exister selon les versions de macOS.

---

### 6.5 Linux

Sous Linux, l’objectif est :

```text
FLTK statique
dépendances système minimales
binaire aussi simple que possible
```

Il est acceptable que certaines bibliothèques système restent dynamiques :

* libc ;
* libX11 ;
* libfontconfig ;
* libXft ;
* libpthread ;
* bibliothèques graphiques système nécessaires.

Un binaire Linux totalement statique n’est pas l’objectif prioritaire, car cela peut être fragile selon les distributions.

---

## 7. Interface graphique

L’interface doit rester simple et adaptée à FLTK.

Elle peut être organisée avec `Fl_Tabs`.

Onglets proposés :

```text
Configuration
Clés
Texte
Fichiers
Journal
```

Widgets FLTK recommandés :

```text
Fl_Window
Fl_Tabs
Fl_Group
Fl_Button
Fl_Input
Fl_Output
Fl_Text_Buffer
Fl_Text_Editor
Fl_Text_Display
Fl_Hold_Browser
Fl_Table_Row
Fl_Native_File_Chooser
```

Pour la version minimale, `Fl_Hold_Browser` suffit pour afficher les clés.

Pour une version plus avancée, `Fl_Table_Row` est préférable.

---

## 8. Onglet Configuration

Cet onglet doit permettre de choisir et tester l’exécutable `gpg`.

Éléments attendus :

* champ affichant le chemin de `gpg` ;
* bouton « Choisir gpg » ;
* bouton « Détecter automatiquement » ;
* bouton « Tester » ;
* affichage de la version détectée ;
* bouton « Recharger les clés ».

Commande utilisée :

```bash
gpg --version
```

L’application doit :

1. permettre la sélection manuelle de l’exécutable ;
2. tenter une détection automatique dans le `PATH` ;
3. tester quelques chemins standards selon la plateforme ;
4. afficher clairement si `gpg` est valide ou invalide ;
5. mémoriser le dernier exécutable sélectionné.

---

## 9. Détection automatique de GPG

L’application doit chercher `gpg`.

### 9.1 Windows

Chemins possibles :

```text
C:\Program Files (x86)\GnuPG\bin\gpg.exe
C:\Program Files\GnuPG\bin\gpg.exe
```

Puis chercher dans le `PATH`.

### 9.2 macOS

Chemins possibles :

```text
/opt/local/bin/gpg
/opt/homebrew/bin/gpg
/usr/local/bin/gpg
/usr/bin/gpg
```

Puis chercher dans le `PATH`.

### 9.3 Linux

Chemins possibles :

```text
/usr/bin/gpg
/usr/local/bin/gpg
/bin/gpg
```

Puis chercher dans le `PATH`.

Une fois trouvé, l’exécutable doit être validé par :

```bash
gpg --version
```

---

## 10. Onglet Clés

L’application doit lister les clés disponibles dans le trousseau GPG de l’utilisateur.

Commandes :

```bash
gpg --list-keys --with-colons --fingerprint --fingerprint
gpg --list-secret-keys --with-colons --fingerprint --fingerprint
```

Le format `--with-colons` doit être parsé proprement.

L’application doit extraire au minimum :

```text
type de clé
empreinte complète
key ID long
UID
adresse e-mail éventuelle
date de création
date d’expiration éventuelle
capacités
validité
état expiré
état révoqué
présence d’une clé secrète
```

L’application doit tenir compte des lignes GPG :

```text
pub
sec
uid
fpr
sub
ssb
```

Les capacités de la clé doivent être utilisées pour déterminer si une clé peut :

```text
chiffrer
signer
certifier
authentifier
```

L’application doit toujours utiliser l’empreinte complète pour appeler `gpg`.

Elle ne doit pas utiliser de key ID court.

---

## 11. Sélection des clés

L’application doit gérer deux clés sélectionnées séparément :

```text
clé de chiffrement
clé de signature
```

Ces deux clés peuvent être identiques, mais elles doivent être traitées comme deux choix fonctionnellement distincts.

---

### 11.1 Clé de chiffrement

La clé de chiffrement sert à chiffrer :

* un fichier ;
* un texte saisi ou collé.

Elle doit correspondre à une clé publique possédant la capacité de chiffrement.

L’application doit afficher uniquement les clés utilisables pour le chiffrement, ou signaler clairement lorsqu’une clé affichée ne possède pas cette capacité.

La clé doit être appelée par son empreinte complète.

Exemple :

```bash
gpg --armor --encrypt --recipient <FINGERPRINT>
```

---

### 11.2 Clé de signature

La clé de signature sert à signer :

* un fichier ;
* un texte saisi ou collé.

Elle doit correspondre à une clé secrète disponible localement et possédant la capacité de signature.

L’application doit afficher uniquement les clés utilisables pour la signature, ou signaler clairement lorsqu’une clé affichée ne possède pas cette capacité.

La clé doit être appelée par son empreinte complète.

Exemple :

```bash
gpg --armor --detach-sign --local-user <FINGERPRINT>
```

---

### 11.3 Mémorisation des clés

Les deux dernières clés sélectionnées doivent être mémorisées séparément dans le fichier de préférences.

Exemple :

```ini
[keys]
encryption_fingerprint=2D267B4F54328AA6AC3B7FE7FDB2A09570A82D34
signing_fingerprint=2D267B4F54328AA6AC3B7FE7FDB2A09570A82D34
```

L’application ne doit jamais mémoriser une clé par :

* son nom affiché ;
* son adresse e-mail ;
* son key ID court.

Elle doit toujours utiliser l’empreinte complète.

---

### 11.4 Restauration au démarrage

Au démarrage, l’application doit :

1. charger les préférences ;
2. relire les clés disponibles avec `gpg` ;
3. rechercher la clé de chiffrement mémorisée par empreinte complète ;
4. rechercher la clé de signature mémorisée par empreinte complète ;
5. restaurer les sélections si les clés existent encore ;
6. ignorer proprement les empreintes absentes ou invalides.

Si une clé précédemment mémorisée n’existe plus, afficher un état neutre :

```text
Aucune clé de chiffrement sélectionnée
Aucune clé de signature sélectionnée
```

sans provoquer d’erreur bloquante.

---

## 12. Export de clé publique

L’application doit permettre d’exporter la clé publique correspondant à la clé de chiffrement sélectionnée.

Fonctions attendues :

* bouton « Exporter la clé publique » ;
* choix du fichier de destination ;
* export ASCII armuré par défaut ;
* extension suggérée `.asc` ou `.txt`.

Commande :

```bash
gpg --armor --export <FINGERPRINT>
```

L’application doit écrire la sortie dans le fichier choisi.

Options souhaitables :

* afficher la clé publique dans une zone de texte ;
* copier la clé publique dans le presse-papiers ;
* enregistrer la clé publique dans un fichier.

---

## 13. Onglet Texte

L’onglet Texte doit permettre de travailler sur du texte saisi ou copié-collé.

Éléments attendus :

* grande zone de texte source ;

* grande zone de texte résultat ;

* choix de l’opération :
  
  * chiffrer ;
  * déchiffrer ;
  * signer ;
  * vérifier ;

* sélection de la clé concernée si nécessaire ;

* bouton « Exécuter » ;

* bouton « Copier le résultat » ;

* bouton « Effacer » ;

* bouton « Enregistrer le résultat ».

Utiliser de préférence :

```text
Fl_Text_Buffer
Fl_Text_Editor
Fl_Text_Display
```

---

## 14. Chiffrement et déchiffrement de texte

### 14.1 Chiffrer un texte

Commande conceptuelle :

```bash
gpg --armor --encrypt --recipient <FINGERPRINT>
```

Le texte clair doit être fourni à `gpg` via stdin.

Le texte chiffré doit être récupéré sur stdout.

L’option ASCII armurée doit être activée par défaut.

---

### 14.2 Déchiffrer un texte

Commande conceptuelle :

```bash
gpg --decrypt
```

Le texte chiffré doit être fourni à `gpg` via stdin.

Le texte déchiffré doit être récupéré sur stdout.

La passphrase éventuelle doit être gérée par `gpg-agent`.

L’application ne doit pas demander elle-même la passphrase.

---

## 15. Signature et vérification de texte

### 15.1 Signer un texte

Mode par défaut :

```text
signature claire ASCII
```

Commande conceptuelle :

```bash
gpg --clearsign --local-user <FINGERPRINT>
```

Le texte doit être fourni sur stdin.

Le texte signé doit être récupéré sur stdout.

---

### 15.2 Vérifier un texte signé

Commande conceptuelle :

```bash
gpg --verify
```

Le texte signé doit être fourni sur stdin.

Le résultat de vérification doit être récupéré depuis stdout/stderr.

L’application doit afficher clairement :

* signature valide ;
* signature invalide ;
* clé inconnue ;
* identité du signataire ;
* empreinte ;
* date de signature ;
* niveau de confiance si disponible.

---

## 16. Onglet Fichiers

L’onglet Fichiers doit permettre de sélectionner :

* un fichier source ;
* un fichier destination ;
* éventuellement un fichier signature.

Utiliser :

```text
Fl_Native_File_Chooser
```

Le dernier répertoire utilisé doit être mémorisé.

L’application doit distinguer :

```text
dernier répertoire d’ouverture
dernier répertoire d’enregistrement
dernier répertoire d’export de clé
dernier répertoire de signature
```

Une version minimale peut commencer avec seulement :

```text
last_open_dir
last_save_dir
```

---

## 17. Chiffrement et déchiffrement de fichiers

### 17.1 Chiffrer un fichier

Commande :

```bash
gpg --yes --batch --encrypt --recipient <FINGERPRINT> --output <DESTINATION> <SOURCE>
```

Avec sortie ASCII :

```bash
gpg --yes --batch --armor --encrypt --recipient <FINGERPRINT> --output <DESTINATION> <SOURCE>
```

---

### 17.2 Déchiffrer un fichier

Commande :

```bash
gpg --yes --decrypt --output <DESTINATION> <SOURCE>
```

La passphrase éventuelle doit être gérée par `gpg-agent`.

---

## 18. Signature et vérification de fichiers

### 18.1 Signer un fichier

Mode recommandé par défaut :

```text
signature détachée ASCII armurée
```

Commande :

```bash
gpg --armor --detach-sign --local-user <FINGERPRINT> --output <SIGNATURE> <SOURCE>
```

Autres modes possibles :

```bash
gpg --clearsign --local-user <FINGERPRINT> --output <DESTINATION> <SOURCE>
gpg --sign --local-user <FINGERPRINT> --output <DESTINATION> <SOURCE>
```

---

### 18.2 Vérifier une signature de fichier

Signature détachée :

```bash
gpg --verify <SIGNATURE> <SOURCE>
```

Fichier signé ou clearsign :

```bash
gpg --verify <SIGNED_FILE>
```

L’application doit afficher clairement :

* signature valide ;
* signature invalide ;
* clé inconnue ;
* clé expirée ;
* signature faite par telle clé ;
* empreinte de la clé ;
* date de signature ;
* UID associé si disponible.

---

## 19. Exécution des processus

Créer un module `GpgProcess`.

Responsabilités :

* lancer `gpg` ;
* passer les arguments sous forme de tableau ;
* éviter toute interpolation shell ;
* fournir éventuellement des données sur stdin ;
* récupérer stdout ;
* récupérer stderr ;
* récupérer le code retour ;
* gérer les erreurs d’exécution.

Règle impérative :

```text
Ne jamais construire une commande shell sous forme d’une chaîne unique.
```

Il faut utiliser les APIs système adaptées.

### 19.1 POSIX / Linux / macOS

Utiliser :

```text
fork
exec
pipe
waitpid
```

ou une abstraction C++ interne équivalente.

### 19.2 Windows

Utiliser :

```text
CreateProcess
pipes
```

Ne pas ajouter une grosse dépendance uniquement pour lancer un processus.

---

## 20. Architecture logicielle

Architecture modulaire recommandée.

### 20.1 `AppInfo`

Responsabilités :

* centraliser le nom de l’application ;
* centraliser l’identifiant projet ;
* centraliser le nom du fichier de préférences ;
* éviter les chaînes codées en dur dans plusieurs modules.

Exemple :

```cpp
namespace AppInfo {
    constexpr const char* Name = "SealKey";
    constexpr const char* FullName = "SealKey — Lightweight GPG desktop helper";
    constexpr const char* ProjectId = "sealkey";
    constexpr const char* PreferencesFile = "preferences.ini";
}
```

---

### 20.2 `GpgExecutable`

Responsabilités :

* stocker le chemin de `gpg` ;
* tester l’exécutable ;
* récupérer la version ;
* valider que le programme répond correctement.

---

### 20.3 `GpgProcess`

Responsabilités :

* lancer `gpg` ;
* gérer stdin/stdout/stderr ;
* récupérer le code retour ;
* éviter le shell ;
* gérer les erreurs d’exécution.

---

### 20.4 `KeyStore`

Responsabilités :

* lister les clés publiques ;
* lister les clés secrètes ;
* parser le format `--with-colons` ;
* fournir des structures `GpgKey`.

---

### 20.5 `GpgKey`

Structure possible :

```cpp
struct GpgKey {
    std::string fingerprint;
    std::string keyId;
    std::string uid;
    std::string email;
    std::string createdAt;
    std::string expiresAt;
    std::string capabilities;
    bool hasSecretKey = false;
    bool canEncrypt = false;
    bool canSign = false;
    bool canCertify = false;
    bool canAuthenticate = false;
    bool revoked = false;
    bool expired = false;
    std::string validity;
};
```

---

### 20.6 `CryptoService`

Responsabilités :

* chiffrer un fichier ;
* déchiffrer un fichier ;
* chiffrer un texte ;
* déchiffrer un texte.

---

### 20.7 `SignatureService`

Responsabilités :

* signer un fichier ;
* vérifier un fichier ;
* signer un texte ;
* vérifier un texte.

---

### 20.8 `SettingsService`

Responsabilités :

* déterminer le chemin du fichier de préférences ;
* lire les préférences ;
* écrire les préférences ;
* fournir des valeurs par défaut ;
* valider les chemins ;
* mémoriser la géométrie de fenêtre ;
* mémoriser le chemin de `gpg` ;
* mémoriser les empreintes des dernières clés choisies ;
* mémoriser les derniers répertoires utilisés.

---

### 20.9 `MainWindow`

Responsabilités :

* construire l’interface FLTK ;
* connecter les boutons aux services ;
* afficher les résultats ;
* afficher les erreurs ;
* gérer l’état des champs ;
* sauvegarder les préférences lors des changements importants.

---

## 21. Préférences utilisateur

L’application doit mémoriser certaines préférences utilisateur afin d’améliorer le confort d’utilisation d’une session à l’autre.

Ces préférences doivent être stockées dans un fichier local propre à l’utilisateur.

La configuration ne doit jamais contenir :

* passphrase ;
* clé privée ;
* texte clair saisi ;
* texte chiffré ;
* signature ;
* contenu de fichier ;
* résultat complet d’une opération GPG ;
* donnée cryptographique sensible.

---

### 21.1 Données à mémoriser

L’application doit mémoriser au minimum :

```text
dernière position de la fenêtre
dernière taille de la fenêtre
dernier état de fenêtre : normal ou maximized
dernier onglet ouvert
dernier exécutable gpg sélectionné
dernière clé de chiffrement sélectionnée
dernière clé de signature sélectionnée
dernier répertoire utilisé pour ouvrir un fichier
dernier répertoire utilisé pour enregistrer un fichier
dernier répertoire utilisé pour exporter une clé publique
dernier répertoire utilisé pour ouvrir une signature
dernier répertoire utilisé pour enregistrer une signature
préférence ASCII armor
type de signature préféré
```

Les clés doivent être mémorisées par empreinte complète.

---

### 21.2 Emplacements du fichier de préférences

Le fichier de préférences doit être stocké dans un emplacement standard selon la plateforme.

#### Linux

Utiliser en priorité :

```text
$XDG_CONFIG_HOME/sealkey/preferences.ini
```

Si `XDG_CONFIG_HOME` n’est pas défini :

```text
~/.config/sealkey/preferences.ini
```

#### macOS

Utiliser :

```text
~/Library/Application Support/sealkey/preferences.ini
```

#### Windows

Utiliser :

```text
%APPDATA%\sealkey\preferences.ini
```

Le nom de l’application, l’identifiant projet et le nom du fichier de préférences doivent être centralisés dans des constantes applicatives.

---

### 21.3 Format du fichier

Utiliser un format simple, lisible et modifiable à la main.

Format recommandé :

```text
INI
```

Éviter d’ajouter une dépendance JSON externe.

Exemple :

```ini
[window]
x=120
y=80
width=980
height=720
state=normal
last_tab=keys

[gpg]
executable=/opt/local/bin/gpg

[keys]
encryption_fingerprint=2D267B4F54328AA6AC3B7FE7FDB2A09570A82D34
signing_fingerprint=2D267B4F54328AA6AC3B7FE7FDB2A09570A82D34

[paths]
last_open_dir=/Users/alexandre/Documents
last_save_dir=/Users/alexandre/Documents
last_key_export_dir=/Users/alexandre/Documents
last_signature_open_dir=/Users/alexandre/Documents
last_signature_save_dir=/Users/alexandre/Documents

[options]
armor=true
signature_type=detached_ascii
```

---

### 21.4 Chargement des préférences

Au démarrage, l’application doit :

1. localiser le fichier de préférences ;
2. le lire si présent ;
3. restaurer la taille et la position de la fenêtre ;
4. vérifier que la fenêtre reste visible à l’écran ;
5. restaurer le dernier onglet ouvert ;
6. restaurer le dernier chemin `gpg` ;
7. tester l’exécutable `gpg` ;
8. charger les clés ;
9. resélectionner les dernières clés connues si elles existent encore ;
10. restaurer les derniers répertoires utilisés ;
11. restaurer les options utilisateur.

Si une valeur mémorisée n’est plus valide, l’application doit l’ignorer proprement.

Exemples :

* l’exécutable `gpg` n’existe plus ;
* la clé mémorisée n’est plus disponible ;
* le dernier répertoire utilisé a été supprimé ;
* la position de fenêtre est hors écran ;
* le fichier de préférences est corrompu.

Dans ces cas, l’application doit revenir à des valeurs par défaut sans planter.

---

### 21.5 Sauvegarde des préférences

L’application doit sauvegarder les préférences :

* à la fermeture normale ;
* après changement de l’exécutable `gpg` ;
* après changement de clé de chiffrement ;
* après changement de clé de signature ;
* après utilisation d’un dialogue de fichier ;
* après changement d’option importante.

La sauvegarde doit être robuste :

1. écrire dans un fichier temporaire ;
2. vérifier que l’écriture a réussi ;
3. remplacer atomiquement l’ancien fichier si possible.

Cela évite de corrompre le fichier de préférences en cas d’arrêt brutal.

---

### 21.6 Position de fenêtre

L’application doit mémoriser :

```text
x
y
width
height
state
```

Le champ `state` peut valoir :

```text
normal
maximized
```

Au démarrage, avant de restaurer la position, l’application doit vérifier que la fenêtre reste visible sur un écran disponible.

Si la position mémorisée semble invalide ou hors écran, utiliser une position par défaut centrée ou proche du centre.

---

### 21.7 Structures C++ suggérées

```cpp
struct WindowSettings {
    int x = -1;
    int y = -1;
    int width = 980;
    int height = 720;
    std::string state = "normal";
    std::string lastTab = "configuration";
};

struct GpgSettings {
    std::string executablePath;
};

struct KeySettings {
    std::string encryptionFingerprint;
    std::string signingFingerprint;
};

struct PathSettings {
    std::string lastOpenDir;
    std::string lastSaveDir;
    std::string lastKeyExportDir;
    std::string lastSignatureOpenDir;
    std::string lastSignatureSaveDir;
};

struct OptionSettings {
    bool armor = true;
    std::string signatureType = "detached_ascii";
};

struct AppSettings {
    WindowSettings window;
    GpgSettings gpg;
    KeySettings keys;
    PathSettings paths;
    OptionSettings options;
};
```

Interface du service :

```cpp
class SettingsService {
public:
    AppSettings load();
    bool save(const AppSettings& settings);

    std::filesystem::path configDirectory() const;
    std::filesystem::path preferencesFile() const;
};
```

---

### 21.8 Permissions

Sous Linux/macOS :

```text
0700 pour le dossier de configuration
0600 pour le fichier de préférences
```

Sous Windows :

```text
utiliser les ACL standards du profil utilisateur dans AppData
```

---

## 22. Sécurité

L’application ne doit jamais :

* stocker de passphrase ;
* demander elle-même la passphrase ;
* enregistrer automatiquement du texte clair ;
* envoyer des données sur le réseau ;
* modifier le trousseau GPG sans action explicite ;
* supprimer une clé ;
* importer une clé sans confirmation explicite ;
* conserver un historique des textes traités ;
* conserver un historique des fichiers traités contenant des données sensibles.

L’application doit :

* utiliser `gpg-agent` ;
* afficher les empreintes complètes pour les opérations sensibles ;
* avertir si une clé est expirée ;
* avertir si une clé est révoquée ;
* avertir si une signature est valide cryptographiquement mais que la clé n’est pas de confiance ;
* supprimer les fichiers temporaires ;
* éviter autant que possible d’écrire du texte clair sur disque.

---

## 23. Presse-papiers

L’application peut proposer un bouton « Copier ».

Elle doit documenter que le presse-papiers peut être lu par d’autres applications.

Options souhaitables :

* bouton « Effacer le presse-papiers » ;
* option pour effacer automatiquement le presse-papiers après un délai.

Ne jamais copier automatiquement une donnée sensible dans le presse-papiers sans action explicite de l’utilisateur.

---

## 24. Fichiers temporaires

L’objectif est d’éviter les fichiers temporaires pour les opérations texte en utilisant stdin/stdout.

Si des fichiers temporaires sont nécessaires :

* utiliser un nom non prédictible ;
* restreindre les permissions ;
* supprimer le fichier après usage ;
* ne jamais conserver de copie cachée du texte clair.

Sous Unix/macOS, utiliser des permissions équivalentes à :

```text
0600
```

Sous Windows, utiliser les ACL du profil utilisateur.

---

## 25. Gestion des erreurs

L’application doit gérer proprement :

* `gpg` introuvable ;
* exécutable `gpg` invalide ;
* version GPG non compatible ;
* absence de clé publique ;
* absence de clé secrète ;
* clé sélectionnée non utilisable pour l’opération demandée ;
* fichier introuvable ;
* permission refusée ;
* passphrase annulée ;
* signature invalide ;
* clé inconnue ;
* clé expirée ;
* clé révoquée ;
* sortie GPG inattendue ;
* fichier de préférences corrompu ;
* dernier répertoire inexistant ;
* position de fenêtre hors écran.

Messages exemples :

```text
Aucune clé publique utilisable pour le chiffrement n’a été trouvée.
Cette clé ne possède pas de capacité de signature.
La signature est cryptographiquement valide, mais la clé du signataire n’est pas marquée comme fiable.
GPG a annulé l’opération, probablement parce que la passphrase n’a pas été fournie ou a été refusée.
L’exécutable GPG sélectionné n’est plus disponible.
La clé mémorisée dans les préférences n’existe plus dans le trousseau GPG.
```

---

## 26. Journal

L’application peut fournir un onglet Journal.

Le journal doit afficher :

* date et heure ;
* opération ;
* résultat ;
* message simplifié ;
* code retour éventuel ;
* empreinte de clé utilisée si utile.

Le journal ne doit jamais contenir :

* passphrase ;
* texte clair ;
* contenu chiffré complet ;
* signature complète ;
* contenu de fichier.

Le journal est destiné au diagnostic léger, pas à l’audit cryptographique complet.

---

## 27. Build CMake

Structure recommandée :

```text
sealkey/
  CMakeLists.txt
  README.md
  src/
    main.cpp
    MainWindow.cpp
    GpgExecutable.cpp
    GpgProcess.cpp
    KeyStore.cpp
    CryptoService.cpp
    SignatureService.cpp
    SettingsService.cpp
    IniFile.cpp
    AppInfo.cpp
  include/
    MainWindow.hpp
    GpgExecutable.hpp
    GpgProcess.hpp
    KeyStore.hpp
    GpgKey.hpp
    CryptoService.hpp
    SignatureService.hpp
    SettingsService.hpp
    IniFile.hpp
    AppInfo.hpp
  third_party/
    fltk/
  tests/
    test_key_parser.cpp
    test_settings.cpp
    test_gpg_arguments.cpp
```

Options CMake souhaitées :

```text
BUILD_STATIC=ON
USE_SYSTEM_FLTK=OFF
ENABLE_TESTS=ON
```

Objectif du build par défaut :

```text
FLTK statique
pas de dépendance graphique exotique externe
pas de libgpgme
appel direct à gpg
```

---

## 28. Tests

Prévoir des tests unitaires pour :

* parsing de `--with-colons` ;
* détection des capacités de clé ;
* construction des arguments GPG ;
* gestion des erreurs ;
* lecture/écriture de configuration ;
* validation du fichier INI ;
* restauration des préférences absentes ou invalides.

Prévoir des tests d’intégration optionnels avec un répertoire GPG temporaire :

```bash
GNUPGHOME=<temporary-directory>
```

Les tests d’intégration peuvent générer une clé de test non protégée par passphrase pour valider :

* chiffrement ;
* déchiffrement ;
* signature ;
* vérification ;
* export de clé publique.

Aucune clé réelle de l’utilisateur ne doit être utilisée dans les tests automatisés.

---

## 29. Priorité de développement

### 29.1 Version 0.1 — Prototype minimal

Objectif : application graphique FLTK minimale fonctionnelle.

Fonctions obligatoires :

* fenêtre principale FLTK ;
* restauration et sauvegarde de la position de fenêtre ;
* restauration et sauvegarde de la taille de fenêtre ;
* onglet Configuration ;
* sélection de l’exécutable `gpg` ;
* mémorisation du dernier exécutable `gpg` sélectionné ;
* test `gpg --version` ;
* onglet Clés ;
* lecture des clés publiques ;
* lecture des clés secrètes ;
* affichage des clés utilisables pour le chiffrement ;
* affichage des clés utilisables pour la signature ;
* sélection d’une clé de chiffrement ;
* sélection d’une clé de signature ;
* mémorisation séparée des deux empreintes complètes ;
* export de la clé publique correspondant à la clé de chiffrement sélectionnée ;
* mémorisation du dernier répertoire utilisé pour ouvrir ou enregistrer un fichier ;
* fichier de préférences INI.

---

### 29.2 Version 0.2 — Opérations texte

Fonctions :

* onglet Texte ;
* chiffrer un texte ;
* déchiffrer un texte ;
* signer un texte ;
* vérifier un texte signé ;
* copier le résultat ;
* effacer les zones de texte ;
* enregistrer le résultat dans un fichier.

---

### 29.3 Version 0.3 — Opérations fichiers

Fonctions :

* onglet Fichiers ;
* chiffrer un fichier ;
* déchiffrer un fichier ;
* signer un fichier ;
* vérifier une signature de fichier ;
* mémoriser les derniers répertoires utilisés.

---

### 29.4 Version 0.4 — Sécurité et ergonomie

Fonctions :

* meilleurs messages d’erreur ;
* avertissements sur les clés expirées ;
* avertissements sur les clés révoquées ;
* avertissements sur les clés non fiables ;
* journal ;
* nettoyage du presse-papiers ;
* préférences utilisateur complètes ;
* vérification que la fenêtre restaurée reste visible à l’écran.

---

### 29.5 Version 1.0 — Version distribuable

Fonctions :

* builds Windows/Linux/macOS ;
* documentation utilisateur ;
* icône ;
* packaging minimal ;
* tests d’intégration ;
* binaires aussi autonomes que possible ;
* README clair ;
* instructions de compilation par plateforme.

---

## 30. Demande initiale à Codex

Créer un projet **C++20 + CMake + FLTK** nommé **SealKey**.

Le nom complet affiché dans l’application doit être :

```text
SealKey — Lightweight GPG desktop helper
```

SealKey est une petite application graphique multiplateforme servant d’interface à `gpg`.

Contraintes impératives :

1. utiliser FLTK comme toolkit graphique ;

2. privilégier la liaison statique de FLTK ;

3. éviter Qt, wxWidgets, GTK, Electron et libgpgme ;

4. appeler l’exécutable `gpg` externe ;

5. ne jamais utiliser de commande shell construite sous forme de chaîne unique ;

6. passer les arguments à `gpg` proprement ;

7. ne jamais stocker de passphrase ;

8. ne jamais stocker de secret ;

9. créer un fichier de préférences INI local ;

10. mémoriser la position et la taille de fenêtre ;

11. mémoriser le dernier exécutable `gpg` sélectionné ;

12. mémoriser séparément la clé de chiffrement et la clé de signature ;

13. mémoriser les derniers répertoires utilisés ;

14. commencer par une version 0.1 limitée à :
    
    * choix de l’exécutable `gpg` ;
    * test de version ;
    * liste des clés publiques et secrètes ;
    * sélection d’une clé de chiffrement ;
    * sélection d’une clé de signature ;
    * export de la clé publique sélectionnée ;
    * préférences utilisateur.

La première réponse attendue de Codex doit :

1. proposer l’arborescence du projet ;
2. créer le `CMakeLists.txt` ;
3. intégrer FLTK en statique si possible ;
4. créer une fenêtre FLTK minimale avec onglets ;
5. implémenter le module `AppInfo` ;
6. implémenter le module `SettingsService` ;
7. implémenter la sélection et le test de l’exécutable `gpg` ;
8. implémenter le listing des clés ;
9. implémenter la sélection séparée des clés de chiffrement et de signature ;
10. implémenter l’export de clé publique ;
11. documenter les limites de la version 0.1.

Le code doit être modulaire, portable, simple à compiler, et éviter toute dépendance inutile.
