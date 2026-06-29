# Kinesis-Smart-Lock
Code Arduino (ESP32) pour le système de contrôle d'accès biométrique et RFID Kinesis
# Kinesis - Système Centralisé de Contrôle d'Accès Intelligent

## 📝 Présentation du Projet
**Kinesis** est une solution technologique conçue pour moderniser, automatiser et sécuriser la gestion des accès aux infrastructures de l'Institut Supérieur de Management (ISM) de Dakar. 

L'établissement dispose d'un bâtiment de 8 étages combinant bureaux administratifs et salles de classe. Face à la lourdeur d'un processus de verrouillage 100% manuel opéré par les agents de sécurité, **Kinesis** apporte une réponse fluide, dynamique et robuste afin de supprimer la dépendance humaine et d'éliminer les retards impactant le calendrier pédagogique.

## 🛠️ Mon Rôle : Concepteur CAO & Intégrateur Matériel
Au sein d'une équipe pluridisciplinaire, mon périmètre s'est concentré sur l'architecture physique du produit :
* **Conception CAO 3D :** Modélisation complète du châssis, de la coque externe et des structures de fixation internes du boîtier à l'aide de logiciels de modélisation 3D (FreeCAD), en optimisant les tolérances géométriques au millimètre près.
* **Intégration Matérielle :** Réalisation de l'intégralité du câblage physique, de la distribution d'énergie et de l'interconnexion des composants au sein du prototype fonctionnel.

## ⚙️ Spécifications Techniques & Composants
Ce dépôt contient le code source embarqué développé pour le microcontrôleur principal. L'architecture matérielle intègre :
* **Microcontrôleur :** ESP32 (gestion des interfaces et des flux réseau).
* **Authentification Biométrique :** Capteur d'empreintes digitales AS608.
* **Authentification RFID :** Module de lecture de badges sans contact.
* **Gestion d'Énergie :** Alimentation autonome par berceaux de batteries Li-Ion 18650.
* **Interface Utilisateur :** Écran d'affichage d'état et indicateurs LED.

## 📐 Dimensions et Contraintes Mécaniques Validées
* **Dimensions globales :** 100 mm (Largeur) x 190 mm (Longueur) x 45 mm (Profondeur).
* **Épaisseur des parois :** 3 mm pour assurer une robustesse optimale en milieu scolaire.
* **Architecture d'assemblage :** Intégration par couches modulaires avec tolérance de jeu fonctionnel de 0,3 mm pour l'impression 3D, facilitant la maintenance sur site.
* **Visserie :** Plots internes taraudés pour vis M3 (cartes électroniques) et fixations murales renforcées pour vis M4.

---
*Projet académique réalisé dans le cadre de la troisième année de Licence en Informatique Appliquée (Spécialisation Électronique, Télécommunications et Systèmes Embarqués - ETSE) à l'ISM Dakar.*
