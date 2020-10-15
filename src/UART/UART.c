/*
 * uart.c
 *
 *  Created on: 26.06.2020
 *      Author: Piotr
 */

#include "UART.h"




volatile uint8_t ascii_line;


// definiujemy w koñcu nasz bufor UART_RxBuf
volatile char UART_RxBuf[UART_RX_BUF_SIZE];
// definiujemy indeksy okreœlaj¹ce iloœæ danych w buforze
volatile uint8_t UART_RxHead; // indeks oznaczaj¹cy „g³owê wê¿a”
volatile uint8_t UART_RxTail; // indeks oznaczaj¹cy „ogon wê¿a”



// definiujemy w koñcu nasz bufor UART_RxBuf
volatile char UART_TxBuf[UART_TX_BUF_SIZE];
// definiujemy indeksy okreœlaj¹ce iloœæ danych w buforze
volatile uint8_t UART_TxHead; // indeks oznaczaj¹cy „g³owê wê¿a”
volatile uint8_t UART_TxTail; // indeks oznaczaj¹cy „ogon wê¿a”


// wskaŸnik do funkcji callback dla zdarzenia UART_RX_STR_EVENT()
static void (*uart_rx_str_event_callback)(char * pBuf);


// funkcja do rejestracji funkcji zwrotnej w zdarzeniu UART_RX_STR_EVENT()
void register_uart_str_rx_event_callback(void (*callback)(char * pBuf)) {
	uart_rx_str_event_callback = callback;
}


void UART_Conf(uint32_t BaudRate)
{

	USART_InitTypeDef USARTInit;
	GPIO_InitTypeDef GPIOInit;

	//*******************************************************************
	//Ustawienie zegarów mikroprocesora
	//*******************************************************************
	RCC_APB2PeriphClockCmd(RCC_APB2ENR_USART1EN | RCC_APB2ENR_IOPAEN, ENABLE);

	//*******************************************************************
	//Ustawienie pinów mikroprocesora
	//*******************************************************************
	// Konfiguracja PA9 jako Tx
	GPIOInit.GPIO_Pin = GPIO_Pin_9;
	GPIOInit.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIOInit.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIOInit);

	// Konfiguracja PA10 jako Rx
	GPIOInit.GPIO_Pin = GPIO_Pin_10;
	GPIOInit.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIOInit);


	//*******************************************************************
	//Ustawienie komunikacji USART
	//*******************************************************************

	USARTInit.USART_BaudRate = BaudRate;
	USARTInit.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USARTInit.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USARTInit.USART_Parity = USART_Parity_No;
	USARTInit.USART_StopBits = USART_StopBits_1;
	USARTInit.USART_WordLength = USART_WordLength_8b;

	USART_Init(USART1, &USARTInit);
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
	USART_Cmd(USART1, ENABLE);



	//*******************************************************************
	//W³aczenie wektora przerwañ dla USART1
	//*******************************************************************
	//NVIC_EnableIRQ( USART1_IRQn);	// RM -> 205 page -> table "Vector table for other STM32F10xxx device" -> position 37

	NVIC_InitTypeDef NVIC_InitStructure;

  	// Wlacz przerwanie od USART1
  	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
  	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  	NVIC_Init(&NVIC_InitStructure);

}

//***********************************************************************************************
  __attribute__((interrupt)) void USART1_IRQHandler (void)
{

	  if(USART_GetFlagStatus(USART1, USART_IT_RXNE) != RESET)
	  {
		  register uint8_t tmp_head;
		  register char data;

		  data = (char)USART_ReceiveData(USART1);

		  // obliczamy nowy indeks „g³owy wê¿a”
		  tmp_head = ( UART_RxHead + 1) & UART_RX_BUF_MASK;

		  // sprawdzamy, czy w¹¿ nie zacznie zjadaæ w³asnego ogona
		  if ( tmp_head == UART_RxTail )
		  {
			  // tutaj mo¿emy w jakiœ wygodny dla nas sposób obs³u¿yæ  b³¹d spowodowany
			  // prób¹ nadpisania danych w buforze, mog³oby dojœæ do sytuacji gdzie
			  // nasz w¹¿ zacz¹³by zjadaæ w³asny ogon
			  // jednym z najbardziej oczywistych rozwi¹zañ jest np natychmiastowe
			  // wyzerowanie zmiennej ascii_line lub sterowanie sprzêtow¹ lini¹
			  // zajêtoœci bufora
			  UART_RxHead = UART_RxTail;
		  } else
		  {
			  switch( data )
			  {
				  case 0:					// ignorujemy bajt = 0
				  case 10: break;			// ignorujemy znak LF
				  case 13: ascii_line++;	// sygnalizujemy obecnoœæ kolejnej linii w buforze
				  default : UART_RxHead = tmp_head; UART_RxBuf[tmp_head] = data;

				  uart_putc(UART_RxBuf[tmp_head]);
			  }
		  }

		  USART_ClearFlag(USART1, USART_IT_RXNE);
	  }

	  if(USART_GetFlagStatus(USART1, USART_SR_TXE) != RESET)
	  {
		  // sprawdzamy czy indeksy s¹ ró¿ne
		  if ( UART_TxHead != UART_TxTail )
		  {
			  // obliczamy i zapamiêtujemy nowy indeks ogona wê¿a (mo¿e siê zrównaæ z g³ow¹)
			  UART_TxTail = (UART_TxTail + 1) & UART_TX_BUF_MASK;
			  // zwracamy bajt pobrany z bufora  jako rezultat funkcji
			  #ifdef UART_DE_PORT
			  UART_DE_NADAWANIE;
			  #endif

			  USART_SendData(USART1, UART_TxBuf[UART_TxTail]);
			  while(USART_GetFlagStatus(USART1, USART_SR_TC) == RESET);
			  USART_ClearFlag(USART1, USART_SR_TC);
		  }
		  else
		  {
			// zerujemy flagê przerwania wystêpuj¹cego gdy bufor pusty
			  USART_ITConfig(USART1, USART_IT_TXE, DISABLE);
		  }
	  }

}
//***********************************************************************************************
  // Zdarzenie do odbioru danych ³añcucha tekstowego z bufora cyklicznego
  void UART_RX_STR_EVENT(char * rbuf)
  {
  	if( ascii_line ) {
  		if( uart_rx_str_event_callback ) {
  			uart_get_str( rbuf );
  			(*uart_rx_str_event_callback)( rbuf );
  		} else {
  			UART_RxHead = UART_RxTail;
  		}
  	}
  }

  //***********************************************************************************************
  // definiujemy funkcjê dodaj¹c¹ jeden bajt do bufora cyklicznego
  void uart_putc( char data )
  {
  	uint8_t tmp_head;

  		tmp_head  = (UART_TxHead + 1) & UART_TX_BUF_MASK;

      // pêtla oczekuje je¿eli brak miejsca w buforze cyklicznym na kolejne znaki
      while ( tmp_head == UART_TxTail ){}

      UART_TxBuf[tmp_head] = data;
      UART_TxHead = tmp_head;

      // inicjalizujemy przerwanie wystêpuj¹ce, gdy bufor jest pusty, dziêki
      // czemu w dalszej czêœci wysy³aniem danych zajmie siê ju¿ procedura
      // obs³ugi przerwania
      USART_ITConfig(USART1, USART_IT_TXE, ENABLE);
  }

  //***********************************************************************************************
  void uart_puts(char *s)		// wysy³a ³añcuch z pamiêci RAM na UART
  {
    register char c;
    while ((c = *s++)) uart_putc(c);			// dopóki nie napotkasz 0 wysy³aj znak
  }

  //***********************************************************************************************
  void uart_putint(int value, int radix)	// wysy³a na port szeregowy tekst
  {
  	char string[17];			// bufor na wynik funkcji itoa
  	itoa(value, string, radix);		// konwersja value na ASCII
  	uart_puts(string);			// wyœlij string na port szeregowy
  }

  //***********************************************************************************************
  // definiujemy funkcjê pobieraj¹c¹ jeden bajt z bufora cyklicznego
  int uart_getc(void)
  {
  	int data = -1;
      // sprawdzamy czy indeksy s¹ równe
      if ( UART_RxHead == UART_RxTail ) return data;

      // obliczamy i zapamiêtujemy nowy indeks „ogona wê¿a” (mo¿e siê zrównaæ z g³ow¹)
      UART_RxTail = (UART_RxTail + 1) & UART_RX_BUF_MASK;
      // zwracamy bajt pobrany z bufora  jako rezultat funkcji
      data = UART_RxBuf[UART_RxTail];

      return data;
  }

  //***********************************************************************************************
  char * uart_get_str(char * buf)
  {
	  int c;
	  char * wsk = buf;
	  if( ascii_line )
	  {
		while( (c = uart_getc()) )
		{
			if( 13 == c || c < 0) break;
  			*buf++ = c;
  		}
  		*buf=0;
  		ascii_line--;
	  }
	  return wsk;
  }

