
/* Inclusão de bibliotecas */
#include <hiduniversal.h>
#include <usbhub.h>
#include <LiquidCrystal.h>
#include <avr/pgmspace.h>
#include <Usb.h>
#include <usbhub.h>
#include <avr/pgmspace.h>
#include <hidboot.h>

/* Definição das máscaras */
#define DISPLAY_WIDTH 16
#define DIM_CODE      13

/* Definição do pino de saída para rejeição */
int PINO_REJEICAO = 13;

unsigned long TA_Rejeicao = 500;  // Define o tempo de atuação da rejeição (Largura de pulso em milisegundos)
unsigned long Aux_Rejeicao;       // Auxiliar para controle do bit de saída

/* Códigos de barras cadastrados */
uint8_t cadastro_1[DIM_CODE] = { 7, 6, 4, 0, 1, 5, 2, 1, 1, 1, 1, 4, 3 };
uint8_t cadastro_2[DIM_CODE] = { 7, 8, 9, 6, 0, 2, 0, 1, 6, 0, 0, 5, 2 };

/* Variáveis auxiliares de controle */
uint8_t pos_cursor = 0, cnt_erros_1 = 0, cnt_erros_2 = 0;

/* Definição dos pinos do display LCD */
LiquidCrystal lcd(7, 6, 5, 4, 3, 2);
 
USB     Usb;
USBHub     Hub(&Usb);
HIDUniversal      Hid(&Usb);
 
class KbdRptParser : public KeyboardReportParser
{
    void PrintKey(uint8_t mod, uint8_t key); // Print de caracteres ASCII
    protected:
    virtual void OnKeyDown  (uint8_t mod, uint8_t key);
    virtual void OnKeyPressed(uint8_t key);
};
 
void KbdRptParser::OnKeyDown(uint8_t mod, uint8_t key)  
{
    uint8_t c = OemToAscii(mod, key);
 
    if (c)
        OnKeyPressed(c);
}
 
/* Função executada quando um simbolo é recebido */
void KbdRptParser::OnKeyPressed(uint8_t key)  
{
    /* Declaração de variáveis locais */
    static uint32_t next_time = 0;
    static uint8_t current_cursor = 0;
    static uint8_t Aux_CR = 0, 
 
    if( millis() > next_time )
    {
      lcd.clear();          // Limpa display LCD
      current_cursor = 0;   // Reseta posição do cursor - LCD
      pos_cursor = 0;       // Reseta posição do cursor de comparação
      delay( 5 );           // Atraso
      lcd.setCursor( 0,0 ); // Posiciona cursor na linha 0 - Coluna 0 do LCD
    }
 
    next_time = millis() + 200;  //reset watchdog
 
    /* Posiciona cursor em Linha 1 - Coluna 0 caso a linha 0 esteja completa */
    if( current_cursor++ == ( DISPLAY_WIDTH + 1 ))  lcd.setCursor( 0,1 );
 
    /* Printa os símbolos recebidos */
    Serial.print( (char)key );  // Exibe os símbolos em ASCII - Serial
    lcd.print( (char)key );     // Exibe os símbolos em ASCII - LCD

    /* Guarda o símbolo numa variável auxiliar */
    Aux_CR = key;

    /* Se o caractere lido não for um nº, nem um Carriage Return (Terminação) */
    if ((Aux_CR > 47) && (Aux_CR < 58) && (Aux_CR != 13))
    {
        /* Consulta um a um os caracteres lidos com os da lista 1 (Produto 1) */
        if ((Aux_CR - 48) != cadastro_1[pos_cursor]) cnt_erros_1++;  // Caso seja diferente, incrementa contagem de erros

        /* Consulta um a um os caracteres lidos com os da lista 2 (Produto 2) */
        if ((Aux_CR - 48) != cadastro_2[pos_cursor]) cnt_erros_2++;  // Caso seja diferente, incrementa contagem de erros
    }

    /* Desloca a posição do cursor a cada símbolo recebido caso não seja um Carriage Return (Terminação) */
    if (Aux_CR != 13) pos_cursor++;
    else
    {
      /* Retorna ACEITO se uma das comparações não contabilizou erros */
      if ((cnt_erros_1 == 0) || (cnt_erros_2 == 0))  Serial.println("\tStatus: ACEITO");
      else
      {
          /* Retorna REJEITADO se o código lido não está cadastrado */
          Serial.println("\tStatus: REJEITADO");
          /* Dispara contador de atuação da saída física para acionamento da rejeição */
          Aux_Rejeicao = millis() + TA_Rejeicao;
      }
      /* Reseta variáveis de controle */
      cnt_erros_1 = 0;
      cnt_erros_2 = 0;
      pos_cursor = 0;
    } 
};
 
KbdRptParser Prs;
 
void setup()
{
    /* Configuração do pino de saída */
    pinMode(PINO_REJEICAO, OUTPUT);     // Define pino como saída digital
    digitalWrite(PINO_REJEICAO, HIGH);  // Define se o pino inicializa LIGADO OU DESLIGADO

    /* Inicialização da porta serial */
    Serial.begin( 115200 );
    Serial.println("Start");
    
    /* Verifica inicialização da porta USB */
    if (Usb.Init() == -1) Serial.println("OSC did not start.");
    delay( 200 );
 
    Hid.SetReportParser(0, (HIDReportParser*)&Prs);
    /* Define dimensões do LCD - Nº de Linhas e colunas */
    lcd.begin(DISPLAY_WIDTH, 2);
    lcd.clear();
    lcd.noAutoscroll();
    lcd.print("Ready");
    delay( 200 );
}
 
/* Loop principal */
void loop()
{
  Usb.Task();
  
  /* Controle de atuação da saída física */
  if (millis() < Aux_Rejeicao) digitalWrite(PINO_REJEICAO, LOW); // Liga saída da rejeição
  else  digitalWrite(PINO_REJEICAO, HIGH);                       // Desliga saída da rejeição

}
