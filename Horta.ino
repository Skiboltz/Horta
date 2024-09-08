#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <EEPROM.h>
#include <Wire.h>

// Referenciação de classes e funções
class horta;
class Menu;
void verificaHorta();
void atualizaMenu();
void desativaIrrigacao();
uint8_t resetEEPROM();

// Configurações do Display LCD 20x4
LiquidCrystal_I2C lcd(0x27, 20, 4);

// Configurações do teclado
const uint8_t linhas = 4;   // número de linhas do teclado
const uint8_t colunas = 4;  // número de colunas do teclado

const uint8_t portasLinhas[linhas] = { 7, 6, 5, 4 };    // portas conectadas às linhas
const uint8_t portasColunas[colunas] = { 3, 2, 1, 0 };  // portas conectadas às colunas

const char teclas[linhas][colunas] = {
  { '1', '2', '3', 'A' },
  { '4', '5', '6', 'B' },
  { '7', '8', '9', 'C' },
  { '*', '0', '#', 'D' }
};

Keypad teclado = Keypad(makeKeymap(teclas), portasLinhas, portasColunas, linhas, colunas);  // Inicialização do teclado

// Definições da horta
const uint8_t sensorLuminosidade = A0;

uint8_t NUM_HORTAS = (EEPROM.read(0) != 255) ? EEPROM.read(0) : resetEEPROM();  // Caso nunca tenha sido salvo valores na EEPROM, é feito o reset
// Arrays dinâmicos inicializados na função de carregamento da EEPROM
uint8_t* Sensores_Umidade;
uint8_t* Rele;
uint8_t* LEDs;
uint8_t* Umidade_Minima;
uint16_t* tempo_de_irrigacao;
bool* ativaLED;
horta** niveis;

bool bombaAcionada = false;  // Varíavel de controle para acionar apenas uma bomba por vez

// Classe de controle para cada nível
class horta {
private:
  unsigned long ultimaAtualizacao = 0;

public:
  uint8_t sensorUmidade, led, rele;
  uint8_t umidadeMinima;
  bool irrigacao = false;
  bool ativaLed;
  uint8_t umidade = 0;
  uint16_t tempoIrrigacao;

  horta(uint8_t Sensor_Umidade, uint8_t Rele, uint8_t LED, bool ativaLED, uint8_t UmidadeMinima, uint16_t tempoDeIrrigacao)
    : sensorUmidade(Sensor_Umidade), led(LED), rele(Rele), ativaLed(ativaLED), umidadeMinima(UmidadeMinima), tempoIrrigacao(tempoDeIrrigacao) {}

  void loop() {
    // Atualiza a leitura dos sensores
    umidade = 120 - map(analogRead(sensorUmidade), 0, 1023, 0, 100);
    uint8_t luminosidade = map(analogRead(sensorLuminosidade), 0, 1023, 0, 100);

    if (irrigacao) {
      if (umidade >= (umidadeMinima + 20)) {  // Irriga até que a umidade seja 20% acima da umidade mínima
        digitalWrite(rele, HIGH);
        bombaAcionada = false;
        irrigacao = false;
      } else if ((millis() - ultimaAtualizacao) < tempoIrrigacao) {
        digitalWrite(rele, LOW);  // Ativa a bomba de irrigação pelo tempo definido
      } else if ((millis() - ultimaAtualizacao) >= (tempoIrrigacao + 5000)) {
        ultimaAtualizacao = millis();  // Reinicia o loop de irrigação após o tempo de irrigação e um intervalo de 5 segundos
      } else {
        digitalWrite(rele, HIGH);  // Desativa a bomba por 5 segundos
      }
    } else if (umidade <= umidadeMinima && !bombaAcionada) {  // Ativa a irrigação somente se não houver outra irrigação ativa
      irrigacao = true;
      bombaAcionada = true;
      ultimaAtualizacao = millis();
    }

    digitalWrite(led, (luminosidade >= 60 && ativaLed) ? HIGH : LOW);  // Manipula o LED baseado na luminosidade
  }
};

// Funções para salvar e carregar dados da EEPROM
void salvarEEPROM() {
  EEPROM.update(0, NUM_HORTAS);
  uint16_t endereco = 1;  // Variável para gerenciar os endereços de gravação
  for (uint8_t i = 0; i < NUM_HORTAS; i++) {
    EEPROM.update(endereco++, Sensores_Umidade[i]);  // É lido o valor atual do endereço e logo em seguida o endereço é incrimentado
    EEPROM.update(endereco++, Rele[i]);
    EEPROM.update(endereco++, LEDs[i]);
    EEPROM.update(endereco++, ativaLED[i]);
    EEPROM.update(endereco++, Umidade_Minima[i]);
    EEPROM.put(endereco, tempo_de_irrigacao[i]);
    endereco += sizeof(tempo_de_irrigacao[i]);  // Incrimenta o tamanho necessário para o tipo de varíavel (uint16_t possui 2 bytes, portanto é somado 2)
  }
}

void carregarEEPROM() {
  // Inicializa os arrays
  Sensores_Umidade = new uint8_t[NUM_HORTAS];
  Rele = new uint8_t[NUM_HORTAS];
  LEDs = new uint8_t[NUM_HORTAS];
  Umidade_Minima = new uint8_t[NUM_HORTAS];
  tempo_de_irrigacao = new uint16_t[NUM_HORTAS];
  ativaLED = new bool[NUM_HORTAS];
  niveis = new horta*[NUM_HORTAS];

  // Atribui valores aos arrays
  uint16_t endereco = 1;
  for (uint8_t i = 0; i < NUM_HORTAS; i++) {
    Sensores_Umidade[i] = EEPROM.read(endereco++);
    Rele[i] = EEPROM.read(endereco++);
    LEDs[i] = EEPROM.read(endereco++);
    ativaLED[i] = EEPROM.read(endereco++);
    Umidade_Minima[i] = EEPROM.read(endereco++);
    EEPROM.get(endereco, tempo_de_irrigacao[i]);
    endereco += sizeof(tempo_de_irrigacao[i]);
  }
  for (uint8_t i = 0; i < NUM_HORTAS; i++) {  // Inicializa os ponteiros de objetos com objetos
    niveis[i] = new horta(Sensores_Umidade[i], Rele[i], LEDs[i], ativaLED[i], Umidade_Minima[i], tempo_de_irrigacao[i]);
  }
}

// Reseta a EEPROM com valores padrões
uint8_t resetEEPROM() {
  EEPROM.update(0, 3);  // Reseta o número de níveis (3)
  uint16_t endereco = 1;
  for (uint8_t i = 1; i <= 3; i++) {
    EEPROM.update(endereco++, i + 14);              // reset das entradas dos sensores de umidade de solo (15/A1, 16/A2, 17/A3)
    EEPROM.update(endereco++, i + 7);               // reset das portas dos Relés (8, 9, 10)
    EEPROM.update(endereco++, i + 10);              // reset das portas dos LEDs (11, 12, 13)
    EEPROM.update(endereco++, 1);                   // reset das variáveis de controle do acionamento dos LEDs (true, true, true)
    EEPROM.update(endereco++, 35 + ((i - 1) * 5));  // reset das variáveis de umidade mínima (35, 40, 45)
    EEPROM.put(endereco, 8000);                     // reset das variáveis de tempo de irrigação (8000, 8000, 8000)
    endereco += sizeof(uint16_t);
  }
  return 3;  // retorna o número padrão de níveis no caso de ser chamada pelo operador ternário
}

// Variáveis para gerenciamento dos menus
Menu* menuAtivo;
uint8_t menuAtual = 0;

// Classe base para gerenciar os menus
class Menu {
protected:
  unsigned long ultimaAtualizacao = 0;
  uint8_t selecionaHorta() {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Escolha Horta (1-");
    lcd.print(NUM_HORTAS);
    lcd.print("):");
    lcd.setCursor(0, 3);
    lcd.print("Press. * p/ cancelar");
    lcd.setCursor(0, 2);
    lcd.print("Press # p/ confirmar");

    char escolha[3] = { 0 };
    uint8_t index = 0;
    uint8_t hortaEscolhida;

    while (true) {      // looping while para aguardo da entrada de dados
      verificaHorta();  // Chama o loop dos níveis dentro do looping while
      char tecla = teclado.getKey();
      if (tecla >= '0' && tecla <= '9' && index < 2) {  // Verifica se as teclas foram dígitos
        escolha[index] = tecla;
        index++;
        lcd.setCursor(0, 1);
        lcd.print(escolha);  // Informa o que está sendo digitado no Display
      } else if (tecla == '#') {
        escolha[index] = '\0';                                     // Indica o fim da string
        hortaEscolhida = atoi(escolha) - 1;                        // Função que transforma string de números em números inteiros
        if (hortaEscolhida < 0 || hortaEscolhida >= NUM_HORTAS) {  // Verifica se o número digitado está dentro dos limites
          lcd.clear();
          lcd.setCursor(3, 0);
          lcd.print("Horta invalida");
          desativaIrrigacao();
          delay(3000);

          // Permite reinserir dados
          escolha[0] = '\0';
          escolha[1] = '\0';
          index = 0;
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Escolha Horta (1-");
          lcd.print(NUM_HORTAS);
          lcd.print("):");
          lcd.setCursor(0, 3);
          lcd.print("Press. * p/ cancelar");
          lcd.setCursor(0, 2);
          lcd.print("Press # p/ confirmar");
        } else {
          return hortaEscolhida;  // Retorna a posição da horta se for um número válido
        }
      } else if (tecla == '*') {
        menuAtual = 0;
        atualizaMenu();
        return 255;  // Retorno de cancelamento
      }
    }
  }
public:
  virtual void exibir() = 0;
  virtual void gerenciarEntrada(char tecla) = 0;
};

// Configurações dos menus
class MenuPrincipal : public Menu {
private:
  const uint8_t paginas = (NUM_HORTAS - 1) / 3;  //Paginação caso haja mais de 3 hortas
  uint8_t paginaAtual = 0;
public:
  void exibir() override {
    if (millis() - ultimaAtualizacao >= 3000) {
      lcd.clear();
      for (uint8_t i = 0; i < 3; i++) {
        if (i + (paginaAtual * 3) <= NUM_HORTAS) {
          lcd.setCursor(0, i);
          lcd.print("Horta ");
          lcd.print((i + (paginaAtual * 3)) + 1);
          lcd.print(": ");
          lcd.print(niveis[(i + (paginaAtual * 3))]->umidade);
          lcd.print("%");
        }
      }
      lcd.setCursor(0, 3);
      lcd.print("Press. * p/ Config");
      paginaAtual++;
      if (paginaAtual > paginas) {
        paginaAtual = 0;
      }
      ultimaAtualizacao = millis();
    }
  }

  void gerenciarEntrada(char tecla) override {
    switch (tecla) {
      case '*':
        menuAtual = 1;
        atualizaMenu();
        break;
      default:
        break;
    }
  }
};

class MenuConfiguracoes : public Menu {
private:
  void gerenciaLED() {
    uint8_t hortaEscolhida = selecionaHorta();
    if (hortaEscolhida == 255) {
      return;  // Retorna à tela principal caso a seleção tenha sido cancelada
    }

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("LEDs nvl. ");
    lcd.print(hortaEscolhida + 1);
    lcd.print(":");
    lcd.setCursor(0, 1);
    lcd.print("1.Permite");
    lcd.setCursor(0, 2);
    lcd.print("2.Desativa");
    lcd.setCursor(0, 3);
    lcd.print("'*' p/ cancelar");

    while (true) {
      verificaHorta();
      char tecla = teclado.getKey();
      if (tecla == '1') {
        niveis[hortaEscolhida]->ativaLed = true;
        ativaLED[hortaEscolhida] = true;
        salvarEEPROM();
        lcd.clear();
        lcd.setCursor(7, 0);
        lcd.print("Salvo");
        lcd.setCursor(8, 1);
        lcd.print("com");
        lcd.setCursor(6, 2);
        lcd.print("sucesso");
        desativaIrrigacao();  // Desativa a irrigação para evitar excessos de tempo durante o delay
        delay(3000);
        menuAtual = 0;
        atualizaMenu();
        return;
      } else if (tecla == '2') {
        niveis[hortaEscolhida]->ativaLed = false;
        ativaLED[hortaEscolhida] = false;
        salvarEEPROM();
        lcd.clear();
        lcd.setCursor(7, 0);
        lcd.print("Salvo");
        lcd.setCursor(8, 1);
        lcd.print("com");
        lcd.setCursor(6, 2);
        lcd.print("sucesso");
        desativaIrrigacao();
        delay(3000);
        menuAtual = 0;
        atualizaMenu();
        return;
      } else if (tecla == '*') {
        menuAtual = 0;
        atualizaMenu();
        return;
      }
    }
  }
  void atualizaUmidade() {
    uint8_t hortaEscolhida = selecionaHorta();
    if (hortaEscolhida == 255) { return; }

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Nova umidade nvl. ");
    lcd.print(hortaEscolhida + 1);
    lcd.print(":");
    lcd.setCursor(0, 3);
    lcd.print("'#' p/ confirmar");
    lcd.setCursor(0, 2);
    lcd.print("'*' p/ cancelar");

    char novaUmidade[3] = { 0 };
    uint8_t index = 0;

    while (true) {
      verificaHorta();
      char tecla = teclado.getKey();
      if (tecla >= '0' && tecla <= '9' && index < 2) {
        novaUmidade[index] = tecla;
        index++;
        lcd.setCursor(0, 1);
        lcd.print(novaUmidade);
      } else if (tecla == '#') {
        novaUmidade[index] = '\0';
        uint8_t umidadeInt = atoi(novaUmidade);
        if (umidadeInt < 30 || umidadeInt > 60) {
          lcd.clear();
          lcd.setCursor(2, 0);
          lcd.print("Umidade Invalida");
          lcd.setCursor(1, 1);
          lcd.print("Min. 30 - Max. 60");
          desativaIrrigacao();
          delay(3000);
          novaUmidade[0] = '\0';
          novaUmidade[1] = '\0';
          index = 0;
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Nova umidade nvl. ");
          lcd.print(hortaEscolhida + 1);
          lcd.print(":");
          lcd.setCursor(0, 3);
          lcd.print("'#' p/ confirmar");
          lcd.setCursor(0, 2);
          lcd.print("'*' p/ cancelar");
        } else {
          Umidade_Minima[hortaEscolhida] = umidadeInt;
          niveis[hortaEscolhida]->umidadeMinima = umidadeInt;
          salvarEEPROM();
          lcd.clear();
          lcd.setCursor(7, 0);
          lcd.print("Salvo");
          lcd.setCursor(8, 1);
          lcd.print("com");
          lcd.setCursor(6, 2);
          lcd.print("sucesso");
          desativaIrrigacao();
          delay(3000);
          menuAtual = 0;
          atualizaMenu();
          return;
        }
      } else if (tecla == '*') {
        menuAtual = 0;
        atualizaMenu();
        return;
      }
    }
  }

public:
  void exibir() override {
    if (millis() - ultimaAtualizacao >= 5000) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("1.Alterar Umidade");
      lcd.setCursor(0, 1);
      lcd.print("2.Atv./Dstv. LEDs");
      lcd.setCursor(0, 2);
      lcd.print("3.Config. avancadas");
      lcd.setCursor(0, 3);
      lcd.print("*.Voltar");
      ultimaAtualizacao = millis();
    }
  }

  void gerenciarEntrada(char tecla) override {
    switch (tecla) {
      case '1':
        atualizaUmidade();
        break;
      case '2':
        gerenciaLED();
        break;
      case '3':
        menuAtual = 2;
        atualizaMenu();
        break;
      case '*':
        menuAtual = 0;
        atualizaMenu();
        break;
    }
  }
};

class MenuConfigAvancadasPag1 : public Menu {
private:
  void gerenciarHorta() {
    uint8_t hortaEscolhida;
    do {
      hortaEscolhida = selecionaHorta();
      if (hortaEscolhida == 255) { return; }

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Horta ");
      lcd.print(hortaEscolhida + 1);
      lcd.print(": Rele: ");
      lcd.print(Rele[hortaEscolhida]);
      lcd.setCursor(0, 1);
      lcd.print("Sens&Umidade: ");
      lcd.print(Sensores_Umidade[hortaEscolhida]);
      lcd.print("-");
      lcd.print(Umidade_Minima[hortaEscolhida]);
      lcd.print("%");
      lcd.setCursor(0, 2);
      lcd.print("Temp. irrigacao: ");
      lcd.print(tempo_de_irrigacao[hortaEscolhida / 1000]);
      lcd.print("s");
      lcd.setCursor(0, 3);
      lcd.print("LEDs: ");
      lcd.print(LEDs[hortaEscolhida]);
      lcd.print(" - ");
      lcd.print((ativaLED[hortaEscolhida] == 1) ? "Ativo" : "Desativo");
      desativaIrrigacao();
      delay(5000);

    } while (true);
  }

public:
  void exibir() override {
    if (millis() - ultimaAtualizacao >= 5000) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("1.Adicionar nivel");
      lcd.setCursor(0, 1);
      lcd.print("2.Remover nivel");
      lcd.setCursor(0, 2);
      lcd.print("3.Info gerais do nvl");
      lcd.setCursor(0, 3);
      lcd.print("Press 4 p/ prox. pg.");
      ultimaAtualizacao = millis();
    }
  }

  void gerenciarEntrada(char tecla) override {
    switch (tecla) {
      case '1':
        // Código para adicionar nível
        break;
      case '2':
        // Código para remover nível
        break;
      case '3':
        gerenciarHorta();
        break;
      case '4':
        menuAtual = 3;
        atualizaMenu();
        break;
      case '*':
        menuAtual = 0;
        atualizaMenu();
        break;
    }
  }
};

class MenuConfigAvancadasPag2 : public Menu {
private:
  void resetPadrao() {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Deseja resetar todas");
    lcd.setCursor(3, 1);
    lcd.print("configuracoes?");
    lcd.setCursor(0, 2);
    lcd.print("*. Nao");
    lcd.setCursor(0, 3);
    lcd.print("#. Sim");

    while (true) {
      char tecla = teclado.getKey();
      if (tecla == '*') {
        menuAtual = 0;
        atualizaMenu();
        return;
      } else if (tecla == '#') {
        // Libera a memória de todos os arrays
        delete[] Rele;
        delete[] LEDs;
        delete[] Umidade_Minima;
        delete[] Sensores_Umidade;
        delete[] ativaLED;
        delete[] tempo_de_irrigacao;
        for (int i = 0; i < NUM_HORTAS; i++) {
          delete[] niveis[i];
        }
        delete[] niveis;

        NUM_HORTAS = resetEEPROM();  // Reseta todos os valores para valores padrões na EEPROM
        carregarEEPROM();
        lcd.clear();
        lcd.setCursor(2, 0);
        lcd.print("Reset concluido");
        desativaIrrigacao();
        delay(5000);
        menuAtual = 0;
        atualizaMenu();
        return;
      }
    }
  }

  void permutarNivel(uint8_t horta) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Permutar com nivel:");
    lcd.setCursor(0, 2);
    lcd.print("#.Confirmar");
    lcd.setCursor(0, 3);
    lcd.print("*.Cancelar");

    char escolha[3] = { 0 };
    uint8_t index = 0;

    while (true) {
      verificaHorta();
      char tecla = teclado.getKey();

      if (tecla >= '0' && tecla <= '9' && index < 2) {
        escolha[index] = tecla;
        index++;
        lcd.setCursor(0, 1);
        lcd.print(escolha);
      } else if (tecla == '#') {
        escolha[index] = '\0';
        uint8_t permutarCom = atoi(escolha) - 1;
        if (permutarCom < 0 || permutarCom >= NUM_HORTAS || permutarCom == horta) {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Escolha invalida");
          desativaIrrigacao();
          delay(3000);

          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Permutar com nivel:");
          lcd.setCursor(0, 2);
          lcd.print("#.Confirmar");
          lcd.setCursor(0, 3);
          lcd.print("*.Cancelar");
          escolha[0] = '\0';
          escolha[1] = '\0';
          index = 0;
        } else {
          // Troca os valores de niveis[horta] com niveis[permutarCom]
          niveis[horta]->sensorUmidade = niveis[permutarCom]->sensorUmidade;
          niveis[horta]->rele = niveis[permutarCom]->rele;
          niveis[horta]->led = niveis[permutarCom]->led;
          niveis[horta]->umidadeMinima = niveis[permutarCom]->umidadeMinima;
          niveis[horta]->tempoIrrigacao = niveis[permutarCom]->tempoIrrigacao;
          niveis[horta]->ativaLed = niveis[permutarCom]->ativaLed;

          // Troca os valores de niveis[permutarCom] pelos valores das arrays globais na posição de horta
          niveis[permutarCom]->sensorUmidade = Sensores_Umidade[horta];
          niveis[permutarCom]->rele = Rele[horta];
          niveis[permutarCom]->led = LEDs[horta];
          niveis[permutarCom]->umidadeMinima = Umidade_Minima[horta];
          niveis[permutarCom]->tempoIrrigacao = tempo_de_irrigacao[horta];
          niveis[permutarCom]->ativaLed = ativaLED[horta];

          // Salva todos os valores nas arrays globais de suas posições
          Sensores_Umidade[permutarCom] = niveis[permutarCom]->sensorUmidade;
          Rele[permutarCom] = niveis[permutarCom]->rele;
          LEDs[permutarCom] = niveis[permutarCom]->led;
          Umidade_Minima[permutarCom] = niveis[permutarCom]->umidadeMinima;
          tempo_de_irrigacao[permutarCom] = niveis[permutarCom]->tempoIrrigacao;
          ativaLED[permutarCom] = niveis[permutarCom]->ativaLed;

          Sensores_Umidade[horta] = niveis[horta]->sensorUmidade;
          Rele[horta] = niveis[horta]->rele;
          LEDs[horta] = niveis[horta]->led;
          Umidade_Minima[horta] = niveis[horta]->umidadeMinima;
          tempo_de_irrigacao[horta] = niveis[horta]->tempoIrrigacao;
          ativaLED[horta] = niveis[horta]->ativaLed;

          salvarEEPROM();  // Salva as alterações na memória EEPROM

          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Permutacao concluida");
          desativaIrrigacao();
          delay(3000);
          menuAtual = 0;
          atualizaMenu();
          return;
        }
      } else if (tecla == '*') {
        menuAtual = 0;
        atualizaMenu();
        return;
      }
    }
  }

  void modificar(uint8_t horta) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Modificar:1.Rele(");
    lcd.print(Rele[horta]);
    lcd.print(")");
    lcd.setCursor(0, 1);
    lcd.print("2.Sensor Umidade(");
    lcd.print(Sensores_Umidade[horta]);
    lcd.print(")");
    lcd.setCursor(0, 2);
    lcd.print("3.LEDs (");
    lcd.print(LEDs[horta]);
    lcd.print(")");
    lcd.setCursor(0, 3);
    lcd.print("4.Temp irrigacao(");
    lcd.print(tempo_de_irrigacao[horta]);
    lcd.print("s)");

    while (true) {
      verificaHorta();
      char tecla1 = teclado.getKey();
      if (tecla1 == '1') {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Nova porta p/ rele:");
        lcd.setCursor(0, 2);
        lcd.print("#.Confirmar");
        lcd.setCursor(0, 3);
        lcd.print("*.Cancelar");
        lcd.setCursor(0, 1);

        char escolha[3];
        uint8_t index = 0;

        while (true) {
          verificaHorta();
          char tecla2 = teclado.getKey();

          if (tecla2 >= '0' && tecla2 <= '9' && index < 2) {
            escolha[index] = tecla2;
            index++;
            lcd.print(escolha);
          } else if (tecla2 == '#') {
            escolha[index] = '\0';
            uint8_t valorEscolhido = atoi(escolha);
            Rele[horta] = valorEscolhido;
            niveis[horta]->rele = valorEscolhido;
            salvarEEPROM();
            lcd.clear();
            lcd.setCursor(7, 0);
            lcd.print("Salvo");
            lcd.setCursor(8, 1);
            lcd.print("com");
            lcd.setCursor(6, 2);
            lcd.print("sucesso");
            desativaIrrigacao();
            delay(3000);
            return;
          } else if (tecla2 == '*') {
            menuAtual = 0;
            atualizaMenu();
            return;
          }
        }
      } else if (tecla1 == '2') {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Nova porta p/ sensor:");
        lcd.setCursor(0, 2);
        lcd.print("#.Confirmar");
        lcd.setCursor(0, 3);
        lcd.print("*.Cancelar");

        char escolha[3];
        uint8_t index = 0;

        while (true) {
          verificaHorta();
          char tecla2 = teclado.getKey();

          if (tecla2 >= '0' && tecla2 <= '9' && index < 2) {
            escolha[index] = tecla2;
            index++;
            lcd.print(escolha);
          } else if (tecla2 == '#') {
            escolha[index] = '\0';
            uint8_t valorEscolhido = atoi(escolha);
            Sensores_Umidade[horta] = valorEscolhido;
            niveis[horta]->sensorUmidade = valorEscolhido;
            salvarEEPROM();
            lcd.clear();
            lcd.setCursor(7, 0);
            lcd.print("Salvo");
            lcd.setCursor(8, 1);
            lcd.print("com");
            lcd.setCursor(6, 2);
            lcd.print("sucesso");
            desativaIrrigacao();
            delay(3000);
            return;
          } else if (tecla2 == '*') {
            menuAtual = 0;
            atualizaMenu();
            return;
          }
        }
      } else if (tecla1 == '3') {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Nova porta p/ LEDs:");
        lcd.setCursor(0, 2);
        lcd.print("#.Confirmar");
        lcd.setCursor(0, 3);
        lcd.print("*.Cancelar");

        char escolha[3];
        uint8_t index = 0;

        while (true) {
          verificaHorta();
          char tecla2 = teclado.getKey();

          if (tecla2 >= '0' && tecla2 <= '9' && index < 2) {
            escolha[index] = tecla2;
            index++;
            lcd.print(escolha);
          } else if (tecla2 == '#') {
            escolha[index] = '\0';
            uint8_t valorEscolhido = atoi(escolha);
            LEDs[horta] = valorEscolhido;
            niveis[horta]->led = valorEscolhido;
            salvarEEPROM();
            lcd.clear();
            lcd.setCursor(7, 0);
            lcd.print("Salvo");
            lcd.setCursor(8, 1);
            lcd.print("com");
            lcd.setCursor(6, 2);
            lcd.print("sucesso");
            desativaIrrigacao();
            delay(3000);
            return;
          } else if (tecla2 == '*') {
            menuAtual = 0;
            atualizaMenu();
            return;
          }
        }
      } else if (tecla1 == '4') {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Novo temp irrigacao:");
        lcd.setCursor(0, 2);
        lcd.print("#.Confirmar");
        lcd.setCursor(0, 3);
        lcd.print("*.Cancelar");

        char escolha[3];
        uint8_t index = 0;

        while (true) {
          verificaHorta();
          char tecla2 = teclado.getKey();

          if (tecla2 >= '0' && tecla2 <= '9' && index < 2) {
            escolha[index] = tecla2;
            index++;
            lcd.print(escolha);
          } else if (tecla2 == '#') {
            escolha[index] = '\0';
            uint8_t valorEscolhido = atoi(escolha);
            if (valorEscolhido < 5 && valorEscolhido > 60) {
              lcd.clear();
              lcd.setCursor(3, 0);
              lcd.print("Tempo invalido");
              lcd.setCursor(0, 1);
              lcd.print("Min. 5s - Max. 60s");
              desativaIrrigacao();
              delay(3000);

              escolha[0] = '\0';
              escolha[1] = '\0';
              index = 0;

              lcd.clear();
              lcd.setCursor(0, 0);
              lcd.print("Novo temp irrigacao:");
              lcd.setCursor(0, 2);
              lcd.print("#.Confirmar");
              lcd.setCursor(0, 3);
              lcd.print("*.Cancelar");
              lcd.setCursor(0, 1);
            } else {
              tempo_de_irrigacao[horta] = valorEscolhido * 1000;
              niveis[horta]->tempoIrrigacao = valorEscolhido * 1000;
              salvarEEPROM();
              lcd.clear();
              lcd.setCursor(7, 0);
              lcd.print("Salvo");
              lcd.setCursor(8, 1);
              lcd.print("com");
              lcd.setCursor(6, 2);
              lcd.print("sucesso");
              desativaIrrigacao();
              delay(3000);
              return;
            }
          } else if (tecla2 == '*') {
            menuAtual = 0;
            atualizaMenu();
            return;
          }
        }
      } else if (tecla1 == '*') {
        menuAtual = 0;
        atualizaMenu();
        return;
      }
    }
  }
  void modificarNivel() {
    uint8_t hortaEscolhida = selecionaHorta();
    if (hortaEscolhida == 255) { return; }

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("1.Alterar posicao");
    lcd.setCursor(0, 1);
    lcd.print("2.Apenas modificar");
    lcd.setCursor(0, 3);
    lcd.print("*.Sair");

    while (true) {
      verificaHorta();
      char tecla = teclado.getKey();

      if (tecla == '1') {
        permutarNivel(hortaEscolhida);
        return;
      } else if (tecla == '2') {
        modificar(hortaEscolhida);
        return;
      } else if (tecla == '*') {
        menuAtual = 0;
        atualizaMenu();
        return;
      }
    }
  }

public:
  void exibir() override {
    if (millis() - ultimaAtualizacao >= 5000) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("1.Modificar nivel");
      lcd.setCursor(0, 1);
      lcd.print("2.Pag. anterior");
      lcd.setCursor(0, 2);
      lcd.print("3.Resetar config.");
      lcd.setCursor(0, 3);
      lcd.print("Press * p/ sair");
      ultimaAtualizacao = millis();
    }
  }

  void gerenciarEntrada(char tecla) override {
    switch (tecla) {
      case '1':
        modificarNivel();
        break;
      case '2':
        menuAtual = 2;
        atualizaMenu();
        break;
      case '3':
        resetPadrao();
        break;
      case '*':
        menuAtual = 0;
        atualizaMenu();
        break;
    }
  }
};

void atualizaMenu() {
  delete menuAtivo;  // Libera a memória do ponteiro
  // Cria um objeto do menu atual
  if (menuAtual == 0) {
    menuAtivo = new MenuPrincipal();
  } else if (menuAtual == 1) {
    menuAtivo = new MenuConfiguracoes();
  } else if (menuAtual == 2) {
    menuAtivo = new MenuConfigAvancadasPag1();
  } else if (menuAtual == 3) {
    menuAtivo = new MenuConfigAvancadasPag2();
  }
}  // Fim configuração dos menus

// Função para chamar o loop de cada nível
void verificaHorta() {
  for (uint8_t i = 0; i < NUM_HORTAS; i++) {
    niveis[i]->loop();
  }
}

void desativaIrrigacao() {
  bombaAcionada = false;
  for (uint8_t i = 0; i < NUM_HORTAS; i++) {
    niveis[i]->irrigacao = false;
    digitalWrite(Rele[i], HIGH);
  }
}

void setup() {
  lcd.init();
  lcd.backlight();
  lcd.setCursor(7, 0);
  lcd.print("Horta");
  lcd.setCursor(5, 1);
  lcd.print("Automatica");
  lcd.setCursor(3, 2);
  lcd.print("Reprogramavel");

  carregarEEPROM();  // Recarrega todos os dados salvos

  delay(100);

  for (uint8_t i = 0; i < NUM_HORTAS; i++) {
    pinMode(LEDs[i], OUTPUT);
    pinMode(Rele[i], OUTPUT);
    digitalWrite(Rele[i], HIGH);  // Inicia com as bombas desacionadas
  }
  menuAtivo = new MenuPrincipal();

  delay(2900);
  lcd.clear();
}

void loop() {
  verificaHorta();

  char tecla = teclado.getKey();
  if (tecla) {
    menuAtivo->gerenciarEntrada(tecla);
  } else {
    menuAtivo->exibir();
  }
}