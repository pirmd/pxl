# Git Hooks pour PXL

Ce dossier contient des scripts de **Git Hooks** pour automatiser des vérifications et des tâches dans le projet **PXL** (librairie graphique 2D en C99).

## 📌 Configuration

Pour activer ces hooks, exécutez :
```bash
git config core.hooksPath scripts/hooks
```

Cela permet de versionner les hooks dans le dépôt et de les partager avec toute l'équipe.

---

## 🔧 Hooks Disponibles

| Hook | Description | Priorité |
|------|-------------|----------|
| [`pre-commit`](pre-commit) | Vérifie le linting, la compilation et les tests avant chaque commit. | ⭐⭐⭐⭐⭐ |
| [`pre-push`](pre-push) | Teste tous les backends (X11, SDL2) avant un push. | ⭐⭐⭐⭐ |
| [`commit-msg`](commit-msg) | Standardise les messages de commit (format: `type(scope): description`). | ⭐⭐ |
| [`post-merge`](post-merge) | Nettoie les artefacts de build après un `git pull`. | ⭐ |

---

## 🔍 Détails des Hooks

### `pre-commit`
- **Linting** : Exécute `make lint` pour vérifier les avertissements du compilateur (PXL utilise `-Werror`).
- **Build** : Vérifie que le code compile avec `make`.
- **Tests** : Exécute `make test` pour lancer les tests unitaires.

> ⚠️ **Note** : Ce hook peut être lent. Si nécessaire, limitez-le au linting uniquement et utilisez `pre-push` pour les tests lourds.

### `pre-push`
- Exécute `sh test-all.sh` pour tester **tous les backends** (X11, SDL2).
- Utile pour s'assurer que le code fonctionne sur toutes les plateformes supportées.

### `commit-msg`
- Impose un format standardisé pour les messages de commit :
  ```
  type(scope): description
  ```
  Exemples :
  - `feat(gfx): add circle drawing`
  - `fix(text): correct bounds calculation`
  - `refactor(canvas): optimize blit operations`
  - `docs(readme): update build instructions`

- **Types autorisés** : `feat`, `fix`, `refactor`, `docs`, `test`, `chore`.

### `post-merge`
- Nettoie les artefacts de build (fichiers `.o`, `.a`) après un `git pull`.
- Évite les conflits liés aux fichiers générés.

---

## 🚀 Installation

1. **Activer les hooks** (à faire **une fois** par développeur) :
   ```bash
   git config core.hooksPath scripts/hooks
   ```

2. **Vérifier que les hooks sont actifs** :
   ```bash
   git config --get core.hooksPath
   ```
   *(Doit afficher `scripts/hooks`.)*

---

## 📝 Personnalisation

- **Désactiver un hook** : Supprimez-le du dossier `scripts/hooks/` ou renommez-le (ex: `pre-commit.disabled`).
- **Modifier un hook** : Éditez le fichier directement dans `scripts/hooks/`.

---

## 🔗 Références

- [Documentation Git Hooks](https://git-scm.com/docs/githooks)
- [pre-commit.com](https://pre-commit.com/) (pour une gestion avancée des hooks)
