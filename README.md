![SealKey](icons/linux_icons/sealkey_128x128.png "SealKey").

# SealKey

SealKey est une petite application de bureau qui rend GPG plus accessible.

Elle est née d'un constat simple : GPG est puissant, solide et très répandu, mais son usage reste difficile pour le commun des mortels. Même des personnes habituées à piloter des applications en ligne de commande peuvent se tromper dans les options, oublier une étape, confondre chiffrement et signature, ou hésiter au moment de choisir une clé.

SealKey ne remplace pas GPG. Il l'utilise. L'application fournit une interface graphique légère pour les opérations courantes, tout en laissant le vrai travail cryptographique à l'exécutable `gpg` installé sur la machine.

## Ce Que Permet SealKey

SealKey permet de :

- localiser et tester l'exécutable `gpg`;
- créer une clé personnelle;
- importer une clé privée existante dans le trousseau GPG local;
- importer la clé publique d'un correspondant;
- choisir une clé de chiffrement;
- choisir une clé de signature;
- exporter sa clé publique pour la transmettre à un correspondant;
- chiffrer un fichier pour une autre personne;
- déchiffrer un fichier reçu;
- signer un fichier;
- vérifier la signature d'un fichier.

SealKey stocke uniquement des préférences non sensibles, comme la taille de fenêtre, le chemin de `gpg`, les empreintes des clés sélectionnées et les derniers dossiers utilisés. Il ne stocke pas les passphrases, les clés privées, le contenu clair, le contenu chiffré, les signatures ni les sorties complètes de GPG contenant des données utilisateur.

## GPG En Quelques Mots

GPG, pour GNU Privacy Guard, est l'implémentation libre du standard OpenPGP. OpenPGP vient historiquement de PGP, Pretty Good Privacy, un outil devenu une référence pour protéger les échanges numériques.

GPG sert principalement à deux choses :

- **chiffrer** un fichier, souvent appelé "crypter" dans le langage courant, pour que seule la bonne personne puisse le lire;
- **signer** un fichier pour prouver qu'il vient bien de vous et qu'il n'a pas été modifié.

Le chiffrement et la signature reposent sur une paire de clés :

- une **clé publique**, que l'on peut partager;
- une **clé privée**, qui doit rester secrète.

Si Alice veut envoyer un fichier confidentiel à Bob, elle chiffre le fichier avec la clé publique de Bob. Bob le déchiffre ensuite avec sa clé privée.

Si Bob veut prouver qu'un fichier vient bien de lui, il le signe avec sa clé privée. Alice peut vérifier cette signature avec la clé publique de Bob.

## Premier Lancement

La première étape se fait dans l'onglet **Configuration**.

1. Ouvrir l'onglet **Configuration**.
2. Localiser l'exécutable `gpg`, ou utiliser l'auto-détection si elle est disponible.
3. Cliquer sur **Tester** pour vérifier que SealKey peut lancer GPG.
4. Créer une clé personnelle si vous n'en avez pas encore.
5. Exporter votre clé publique pour pouvoir la transmettre à vos correspondants.
6. Importer la clé publique d'un correspondant avec qui vous voulez échanger de manière sécurisée.
7. Sélectionner votre clé privée préférée pour les opérations de signature.

Dans les exemples suivants, Alice et Bob ont chacun leur propre clé. Alice possède sa clé privée et la clé publique de Bob. Bob possède sa clé privée et la clé publique d'Alice.

## Cas D'usage

Les captures d'écran ne sont pas incluses pour l'instant. Elles devront être réalisées avec un trousseau GPG de démonstration, contenant uniquement des clés fictives Alice et Bob, afin de ne pas publier d'identités, d'emails ou d'empreintes de clés réelles.

### 1. Alice Configure SealKey

Alice commence par ouvrir l'onglet **Configuration**.

Elle sélectionne l'exécutable `gpg`, puis lance le test. Si le test réussit, SealKey peut interroger GPG et lister les clés disponibles.

Alice crée ensuite sa clé personnelle si elle n'en possède pas encore. Cette clé lui servira à déchiffrer les fichiers qui lui sont destinés et à signer les fichiers qu'elle envoie.

Alice exporte sa clé publique et l'envoie à Bob. Cette clé publique n'est pas secrète : elle sert justement à permettre à Bob de chiffrer des fichiers pour Alice ou de vérifier ses signatures.

Alice importe aussi la clé publique de Bob. C'est cette clé qui lui permettra de chiffrer un fichier que seul Bob pourra déchiffrer.

### 2. Alice Chiffre Un Fichier Pour Bob

Alice ouvre l'onglet **Crypter**.

Elle choisit le fichier à protéger, puis sélectionne Bob comme destinataire. SealKey utilise l'empreinte complète de la clé publique de Bob pour appeler GPG.

Alice lance le chiffrement. Le résultat est un nouveau fichier chiffré, par défaut avec l'extension `.gpg`.

Alice peut envoyer ce fichier chiffré à Bob par un canal normal : email, messagerie, partage de fichiers ou clé USB. Le fichier peut circuler sans révéler son contenu, car seul Bob possède la clé privée nécessaire pour le déchiffrer.

### 3. Bob Déchiffre Le Fichier Reçu

Bob ouvre SealKey sur sa machine et va dans l'onglet **Décrypter**.

Il sélectionne le fichier chiffré reçu d'Alice, puis lance le déchiffrement. GPG peut demander la passphrase de la clé privée de Bob via le mécanisme standard `gpg-agent` et `pinentry`.

Si la clé privée de Bob correspond bien à la clé publique utilisée par Alice, GPG produit le fichier déchiffré.

SealKey affiche un résultat lisible, avec les informations utiles retournées par GPG, sans enregistrer le contenu du fichier.

### 4. Bob Signe Un Fichier

Bob veut maintenant envoyer un fichier à Alice en lui permettant de vérifier qu'il vient bien de lui.

Il ouvre l'onglet **Signer**, sélectionne le fichier, puis lance la signature avec sa clé privée préférée.

SealKey produit une signature détachée, par défaut avec l'extension `.sig`. Le fichier d'origine n'est pas modifié.

Bob envoie à Alice deux fichiers :

- le fichier original;
- le fichier de signature `.sig`.

### 5. Alice Vérifie La Signature De Bob

Alice ouvre l'onglet **Vérifier**.

Elle sélectionne le fichier reçu, puis la signature `.sig` envoyée par Bob.

SealKey demande à GPG de vérifier la signature. Si Alice possède bien la clé publique de Bob, GPG peut confirmer si la signature est valide.

Une signature valide signifie deux choses :

- le fichier a bien été signé par la clé privée correspondant à la clé publique de Bob;
- le fichier n'a pas été modifié depuis la signature.

Si la clé publique de Bob manque, SealKey indique que la signature ne peut pas être vérifiée tant que cette clé n'a pas été importée.

## Fonctionnalités Actuelles

La version 0.6.1 fournit :

- une application FLTK nommée `sealkey`;
- cinq onglets principaux : **Crypter**, **Décrypter**, **Signer**, **Vérifier** et **Configuration**;
- la sélection, l'auto-détection et le test de l'exécutable `gpg`;
- la liste des clés publiques et privées via `--with-colons`;
- la sélection des clés par empreinte complète;
- l'export ASCII-armored d'une clé publique;
- l'import de clés publiques de correspondants;
- la création de clés privées via `gpg --quick-generate-key`;
- l'import de clés privées existantes via GPG;
- l'export explicite d'une clé privée après confirmation;
- la suppression explicite de clés publiques ou privées après confirmation;
- le chiffrement de fichiers, avec option de signature simultanée;
- le déchiffrement de fichiers;
- la signature détachée de fichiers avec confirmation d'écrasement gérée par SealKey;
- la vérification de signatures détachées;
- des diagnostics plus clairs pour les signatures valides, invalides, inconnues, expirées, révoquées ou non fiables;
- l'affichage des signataires avec date de signature formatée quand GPG la fournit;
- l'affichage chronologique des sorties GPG dans les onglets d'opération, en police monospace, avec messages SealKey en bleu, stdout en noir, stderr en rouge et code de sortie GPG final en magenta;
- la séparation des sorties utilisateur de GPG et du flux technique `--status-fd` utilisé en interne pour identifier les signataires;
- des listes tabulaires avec en-têtes et colonnes redimensionnables, dont les largeurs sont mémorisées;
- la restauration du dernier onglet et de la géométrie de fenêtre;
- des icônes Windows, macOS et Linux, avec intégration Linux via fichier `.desktop`.

L'application appelle toujours `gpg` comme processus externe et transmet les arguments sous forme de vecteur. Elle n'utilise pas Qt, wxWidgets, GTK, Electron, libgpgme ni OpenSSL.

## Compilation Et Lancement

Installez CMake, un compilateur C++20 et FLTK 1.4 ou compatible.

Pour une version avec FLTK embarqué ou statique, placez FLTK dans `third_party/fltk` ou laissez CMake le télécharger :

```bash
cmake -S . -B build -DUSE_SYSTEM_FLTK=OFF -DBUILD_STATIC=ON -DENABLE_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

Un accès réseau peut être nécessaire quand `USE_SYSTEM_FLTK=OFF` et que `third_party/fltk` est absent.

Sur Debian/Ubuntu, quand `USE_SYSTEM_FLTK=OFF`, installez typiquement les dépendances Xft/fontconfig avant de reconstruire FLTK afin d'obtenir un rendu de texte antialiasé :

```bash
sudo apt install libxft-dev libfontconfig1-dev libfreetype-dev libxrender-dev
```

Pour un rendu plus moderne avec FLTK 1.4 et le backend Wayland/Cairo/Pango, installez aussi les dépendances Wayland et les en-têtes X11 requis par le build hybride Wayland/X11 de FLTK :

```bash
sudo apt install libpango1.0-dev libcairo2-dev libwayland-dev wayland-protocols libxkbcommon-dev libdecor-0-dev libxfixes-dev libxcursor-dev libxinerama-dev
```

SealKey n'utilise pas OpenGL ni GTK. Quand `USE_SYSTEM_FLTK=OFF`, le build désactive donc `FLTK_BUILD_GL` et le plugin GTK de libdecor pour éviter les dépendances EGL/OpenGL et GTK inutiles du backend Wayland.

Pour utiliser une installation FLTK déjà disponible sur le système :

```bash
cmake -S . -B build -DUSE_SYSTEM_FLTK=ON -DENABLE_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

### Linux

L'exécutable compilé est disponible ici :

```bash
./build/sealkey
```

Pour une utilisation normale depuis le bureau Linux, installez aussi le fichier `.desktop` et les icônes :

```bash
cmake --install build --prefix "$HOME/.local"
```

Lancez ensuite SealKey depuis le menu d'applications, ou directement :

```bash
"$HOME/.local/bin/sealkey"
```

L'étape d'installation est importante sous Linux, car les barres de tâches et menus d'applications utilisent les fichiers installés :

- `$HOME/.local/share/applications/sealkey.desktop`;
- `$HOME/.local/share/icons/hicolor/.../apps/sealkey.png`;
- `$HOME/.local/share/icons/hicolor/scalable/apps/sealkey.svg`.

Si la barre de tâches affiche encore une ancienne ou une mauvaise icône après réinstallation, rafraîchissez les caches si ces outils sont disponibles :

```bash
update-desktop-database "$HOME/.local/share/applications" 2>/dev/null || true
gtk-update-icon-cache -f -t "$HOME/.local/share/icons/hicolor" 2>/dev/null || true
```

Sur les bureaux légers, notamment Raspberry Pi OS/LXPanel, le panel peut conserver une ancienne association d'icône jusqu'au redémarrage de la session ou du panel. Lancer SealKey depuis un éditeur ou un IDE peut aussi pousser certains panels à associer la fenêtre à l'application parente. Pour tester correctement l'intégration, lancez SealKey depuis l'entrée `sealkey.desktop` installée.

### macOS

Le build CMake crée un bundle d'application macOS. Lancez le bundle, pas l'exécutable brut :

```bash
open build/sealkey.app
```

Depuis le Finder, ouvrez :

```text
build/sealkey.app
```

SealKey n'embarque pas `gpg`. Installez GnuPG séparément, puis sélectionnez l'exécutable `gpg` dans SealKey si l'auto-détection ne le trouve pas. Chemins fréquents :

```text
/opt/homebrew/bin/gpg
/usr/local/bin/gpg
/opt/local/bin/gpg
```

### Windows

Avec un générateur Windows, compilez le projet normalement :

```powershell
cmake -S . -B build -DUSE_SYSTEM_FLTK=OFF -DBUILD_STATIC=ON -DENABLE_TESTS=ON
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

L'exécutable graphique est généré dans le dossier de build. Avec un générateur multi-configuration comme Visual Studio, l'exécutable Release est généralement :

```text
build\Release\sealkey.exe
```

Avec un générateur mono-configuration comme Ninja ou MinGW Makefiles, il est généralement ici :

```text
build\sealkey.exe
```

SealKey n'embarque pas `gpg`. Installez GnuPG séparément, puis sélectionnez `gpg.exe` dans SealKey si l'auto-détection ne le trouve pas.

## Préférences

SealKey stocke uniquement des préférences non sensibles :

- Linux: `$XDG_CONFIG_HOME/sealkey/preferences.ini` ou `~/.config/sealkey/preferences.ini`;
- macOS: `~/Library/Application Support/sealkey/preferences.ini`;
- Windows: `%APPDATA%\sealkey\preferences.ini`.

Les valeurs stockées incluent la géométrie de fenêtre, le dernier onglet, l'exécutable `gpg` sélectionné, les empreintes complètes sélectionnées, les derniers dossiers utilisés, les options d'interface et les extensions de fichiers configurées.

## Limites Actuelles

Les opérations de fichier passent explicitement les chemins source, destination et signature à `gpg` sous forme de vecteur d'arguments.

SealKey ne stocke pas les textes sources, textes chiffrés, signatures, passphrases, contenus de fichiers ni sorties complètes d'opération dans les préférences.

L'interface v0.6 reste volontairement compacte et native. La sélection des destinataires est une liste directe de clés publiques plutôt qu'un carnet d'adresses complet.

Le travail restant avant la version 1.0 concerne surtout la distribution et le polissage : packaging plus complet, tests d'intégration plus larges, glisser-déposer et carnet d'adresses plus riche.

## Licence

SealKey est distribué sous licence BSD 3-Clause. Le texte complet de la licence est disponible dans [LICENSE](LICENSE).

## Crédits

SealKey a été entièrement réalisé avec le concours de ChatGPT et Codex, y compris le graphisme du logo, sous la direction d'Alexandre Vialle.
