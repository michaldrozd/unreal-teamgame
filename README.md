# Jednoduchá Multiplayer FPS Hra (Unreal Engine C++)

Tento projekt predstavuje základnú šablónu pre multiplayerovú strieľačku z pohľadu prvej osoby (FPS) vytvorenú v C++ s použitím Unreal Engine. Demonštruje základné mechaniky ako pohyb, streľba, zdravie, smrť, respawn a zobrazovanie informácií na obrazovke (HUD).

## Funkcionalita

*   **Pohyb postavy:** Pohyb dopredu/dozadu, do strán a otáčanie pohľadu myšou.
*   **Skok:** Základná mechanika skoku.
*   **Streľba:** Strieľanie pomocou raycastu (lúča) z pohľadu kamery. Detekuje zásahy iných hráčov.
*   **Zdravie a Poškodenie:** Postavy majú zdravie, ktoré sa znižuje pri zásahu.
*   **Smrť a Ragdoll:** Po klesnutí zdravia na nulu postava zomrie a aktivuje sa fyzika ragdoll.
*   **Respawn:** Mŕtvy hráč sa po krátkom čase znovu objaví na náhodnom spawn pointe.
*   **Sledovanie Skóre:** Počítanie zabití (Kills) a smrtí (Deaths) pre každého hráča.
*   **Kill Feed:** Zobrazovanie správ o tom, kto koho zabil (alebo kto zomrel).
*   **Základný HUD:** Zobrazenie počtu zabití, smrtí a kill feed správ pomocou UMG (Unreal Motion Graphics).
*   **Replikácia:** Všetky kľúčové akcie a stavy (pohyb, streľba, zdravie, skóre, kill feed) sú replikované pre multiplayer funkčnosť.
*   **Enhanced Input:** Používa nový systém Enhanced Input pre spracovanie vstupov od hráča.

## Hlavné C++ Triedy

*   **`AMyCharacter`**: Reprezentuje postavu hráča. Zodpovedá za pohyb, ovládanie kamery, streľbu, prijímanie poškodenia, smrť (vrátane ragdoll efektu) a spracovanie vstupov cez Enhanced Input. Replikuje svoje zdravie.
*   **`AMyGameMode`**: Definuje pravidlá hry. Beží iba na serveri. Manažuje pripájanie hráčov, vyberanie spawn pointov, proces respawnu po smrti, spracováva udalosť zabitia hráča (aktualizuje štatistiky a kill feed) a odhlásenie hráčov.
*   **`AMyPlayerState`**: Uchováva špecifické dáta pre hráča, ktoré pretrvávajú aj po smrti a respawne postavy (napr. meno, počet zabití, počet smrtí). Tieto dáta sú replikované všetkým klientom. Zodpovedá za aktualizáciu HUDu pri zmene štatistík.
*   **`AMyGameState`**: Uchováva globálny stav hry, ktorý vidia všetci hráči (napr. zoznam správ v kill feede). Tieto dáta sú replikované. Zodpovedá za aktualizáciu kill feed časti HUDu.
*   **`AMyHUD`**: Hlavná trieda pre Heads-Up Display. Vytvára a spravuje inštanciu UMG widgetu (`UMyHUDWidget`). Poskytuje funkcie na aktualizáciu dát zobrazených vo widgete, ktoré volajú `AMyPlayerState` a `AMyGameState`.
*   **`UMyHUDWidget`**: Samotný UMG widget implementovaný v C++. Obsahuje odkazy na textové polia (`UTextBlock`) definované v UMG editore (cez `meta = (BindWidget)`) a poskytuje funkcie na aktualizáciu ich obsahu (zabitia, smrti, kill feed).

## Nastavenie a Použitie

1.  **Kompilácia:** Skopírujte tieto C++ súbory do `Source/<MenoVashoProjektu>` priečinka vášho Unreal Engine projektu a skompilujte projekt pomocou Visual Studia alebo iného podporovaného IDE.
2.  **Blueprinty:** Vytvorte Blueprint triedy odvodené od týchto C++ tried (napr. `BP_MyCharacter` z `AMyCharacter`, `BP_MyGameMode` z `AMyGameMode`, atď.).
3.  **Konfigurácia GameMode:** V nastaveniach projektu (Project Settings -> Maps & Modes) nastavte váš `BP_MyGameMode` ako predvolený Game Mode pre vašu mapu. V `BP_MyGameMode` môžete nastaviť predvolené triedy pre Pawn (`BP_MyCharacter`), PlayerState (`BP_MyPlayerState`), GameState (`BP_MyGameState`) a HUD (`BP_MyHUD`).
4.  **HUD Widget:** Vytvorte UMG Widget Blueprint (napr. `WBP_HUD`) a nastavte jeho rodičovskú triedu na `UMyHUDWidget`. V tomto widgete vytvorte potrebné `TextBlock` prvky a pomenujte ich presne ako premenné v `UMyHUDWidget.h` (`KillCountText`, `DeathCountText`, `KillFeedText`). V `BP_MyHUD` nastavte `HUDWidgetClass` na váš `WBP_HUD`.
5.  **Input Actions:** Vytvorte potrebné Input Action a Input Mapping Context assety v Unreal Editore a priraďte ich v `BP_MyCharacter` (alebo priamo v C++ konštruktore `AMyCharacter`, ak preferujete). Prepojte Input Actions s funkciami v `AMyCharacter::SetupPlayerInputComponent`.
6.  **Spawn Pointy:** Umiestnite do mapy objekty typu `PlayerStart`. `AMyGameMode` ich automaticky nájde a použije na respawn hráčov.
7.  **Testovanie:** Spustite hru v editore s viacerými hráčmi (nastavenie "Number of Players" v Play options) na otestovanie multiplayer funkcionality.
