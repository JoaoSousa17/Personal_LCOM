# 🧠 FightList - Terminal Edition

Este projeto é uma versão em terminal inspirada no jogo **Fight List**, desenvolvido como uma experiência para avaliar a viabilidade da implementação de jogos simples em C++ com interface textual. O objetivo é estudar a facilidade de integração de lógicas deste género num projeto final da unidade curricular de **Laboratórios de Computadores (LCOM)** do curso de **Licenciatura em Engenharia Informática e Computação (LEIC)**.

---

## 🎮 Contexto

Esta versão é **single-player**, jogada contra o tempo, e serve como **prova de conceito** para testar o uso de C++ com menus interativos, utilização de cores ANSI no terminal e gestão de dados através de dicionários temáticos.

---

## 📋 Regras do Jogo

- O jogo escolhe uma **categoria aleatória** (ex: Cores, Frutas, Profissões...).
- O jogador tem **35 segundos** para escrever o máximo de palavras que pertençam a essa categoria.
- Cada palavra tem uma **pontuação entre 1 e 3 pontos**, dependendo da sua raridade.
- O jogo termina quando o tempo esgota ou o jogador acerta todas as palavras da categoria.

---

## 🧱 Estrutura do Projeto

```
FightList/
├── main.cpp          // Menus, entrada principal, função drawBar
├── jogo.cpp          // Lógica do jogo: input, temporizador, pontuação
├── utils.cpp         // Funções utilitárias como drawBar()
├── utils.h
├── dicionarios.cpp   // Definições dos dicionários/categorias
├── dicionarios.h     // Declaração da estrutura Categoria e do vetor global
└── README.md
```

---

## ⚙️ Instruções de Instalação e Execução

### Pré-requisitos
- Compilador C++ com suporte a C++11 (como `g++`)
- Terminal com suporte a **códigos ANSI** (cores)

### Compilar
```bash
g++ main.cpp jogo.cpp utils.cpp dicionarios.cpp -o fightlist -std=c++11
```

### Executar
```bash
./fightlist
```

> No Windows, usar `fightlist.exe` após compilar.

---

## 🗂 Categorias disponíveis

O jogo inclui **10 dicionários** com pelo menos 12 palavras cada:
- Meses do Ano
- Cores
- Frutas
- Profissões
- Espaço
- Animais
- Países Europeus
- Instrumentos Musicais
- Marcas de Carros
- Cidades Portuguesas
