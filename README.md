# 🚪 Simulador de Elevador com Zephyr RTOS

Este projeto é uma aplicação embarcada desenvolvida com o **Zephyr RTOS**, simulando o funcionamento de um elevador. Ele integra diversos recursos do sistema operacional, como drivers de hardware, comunicação entre tarefas e sincronização de threads.

---

## ⚙️ Tecnologias e Recursos Utilizados

- **Drivers GPIO**: Leitura de botões físicos.
- **Driver de Display SSD1306** (I²C): Exibição do status do elevador.
- **Mecanismos de Sincronização**:
  - Mutexes (`k_mutex`) para garantir o acesso seguro a dados compartilhados.
- **Comunicação entre Tarefas**:
  - Fila de Mensagens (`k_msgq`) para envio de requisições dos botões ao elevador.
- **Threads Concorrentes**:
  - Leitura de botões;
  - Controle do elevador;
  - Atualização do display.

---

## 📋 Funcionalidades

- Suporte a **4 andares** (botões físicos de 1 a 4).
- Detecção de chamadas (botões pressionados) com debounce por software.
- Gerenciamento de requisições com fila pendente.
- Movimento do elevador com tempo simulado de **2 segundos por andar**.
- Exibição no display:
  - **Andar atual**;
  - **Andar solicitado** (requisição ativa);
  - **Estado atual** (`R` = Pronto, `M` = Movendo-se);
  - **Fila de andares pendentes**.

---

## 📂 Estrutura do Projeto

```plaintext
boards/
└── esp32_devkitc.overlay    # Device Tree Overlay para ESP32
src/
└── main.c                   # Código-fonte principal do projeto
prj.conf                     # Configurações do Zephyr RTOS
```

---

## ⚙️ Recursos do Zephyr Utilizados

- `k_mutex` → Para proteger acesso à fila de requisições e display.
- `k_msgq` → Comunicação entre botões e a lógica do elevador.
- `k_thread` → Threads independentes:
  - `button_thread`: Lê botões com debounce e envia requisições.
  - `elevator_thread`: Simula movimento entre andares.
  - `display_thread`: Atualiza o display.

---

## 🚀 Como Compilar

1. Compile o projeto:

```bash
west build -b esp32_devkitc/esp32/procpu -p
```

2. Grave o firmware na placa:

```bash
west flash
```

---

## 📖 Documentação

Para mais detalhes sobre o Zephyr RTOS, consulte a [documentação oficial](https://docs.zephyrproject.org/latest/).

---

## 📄 Licença

Este projeto é licenciado sob a licença MIT. Consulte o arquivo [LICENSE](LICENSE) para mais detalhes.

---

## 👨‍💻 Autor

- [Dennis Paz](https://github.com/DenPaz)
