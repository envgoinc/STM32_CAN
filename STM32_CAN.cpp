#include "STM32_CAN.h"

constexpr Baudrate_entry_t STM32_CAN::BAUD_RATE_TABLE_48M[];
constexpr Baudrate_entry_t STM32_CAN::BAUD_RATE_TABLE_45M[];
constexpr Baudrate_entry_t STM32_CAN::BAUD_RATE_TABLE_42M[];

static STM32_CAN* _CAN1 = nullptr;
static CAN_HandleTypeDef     hcan1;
uint32_t test = 0;
#ifdef CAN2
static STM32_CAN* _CAN2 = nullptr;
static CAN_HandleTypeDef     hcan2;
#endif
#ifdef CAN3
static STM32_CAN* _CAN3 = nullptr;
static CAN_HandleTypeDef     hcan3;
#endif

STM32_CAN::STM32_CAN( CAN_TypeDef* canPort, CAN_PINS pins, RXQUEUE_TABLE rxSize, TXQUEUE_TABLE txSize ) {

  if (_canIsActive) { return; }

  sizeRxBuffer=rxSize;
  sizeTxBuffer=txSize;

  if (canPort == CAN1)
  {
    _CAN1 = this;
    n_pCanHandle = &hcan1;
  }
  #ifdef CAN2
  if( canPort == CAN2)
  {
    _CAN2 = this;
    n_pCanHandle = &hcan2;
  }
  #endif
  #ifdef CAN3
  if (canPort == CAN3)
  {
    _CAN3 = this;
    n_pCanHandle = &hcan3;
  }
  #endif

  _canPort = canPort;
  _pins = pins;
}

// Init and start CAN
void STM32_CAN::begin( bool retransmission ) {

  // exit if CAN already is active
  if (_canIsActive) return;

  _canIsActive = true;

  GPIO_InitTypeDef GPIO_InitStruct;

  initializeBuffers();

  // Configure CAN
  if (_canPort == CAN1)
  {
    //CAN1
    __HAL_RCC_CAN1_CLK_ENABLE();

    if (_pins == ALT)
    {
      __HAL_RCC_GPIOB_CLK_ENABLE();
      #if defined(__HAL_RCC_AFIO_CLK_ENABLE)  // Some MCUs like F1xx uses AFIO to set pins, so if there is AFIO defined, we use that.
      __HAL_AFIO_REMAP_CAN1_2();  // To use PB8/9 pins for CAN1.
      __HAL_RCC_AFIO_CLK_ENABLE();
      GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;  // If AFIO is used, there doesn't seem to be "very high" option for speed, so we use "high" -setting.
      GPIO_InitStruct.Pin = GPIO_PIN_8;
      GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
      HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
      GPIO_InitStruct.Pin = GPIO_PIN_9;
      #else  // Without AFIO, this is the way to set the pins for CAN.
      #if defined(GPIO_AF8_CAN1)  // Another weird thing. Depending on the MCU used, this can be AF8 or AF9
      GPIO_InitStruct.Alternate = GPIO_AF8_CAN1;
      #else
      GPIO_InitStruct.Alternate = GPIO_AF9_CAN1;
      #endif
      GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9;
      GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
      #endif
      GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
      HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    }
     if (_pins == DEF)
     {
      __HAL_RCC_GPIOA_CLK_ENABLE();
      GPIO_InitStruct.Pull = GPIO_NOPULL;
      #if defined(__HAL_RCC_AFIO_CLK_ENABLE)
      __HAL_AFIO_REMAP_CAN1_1(); // To use PA11/12 pins for CAN1.
      __HAL_RCC_AFIO_CLK_ENABLE();
      GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
      GPIO_InitStruct.Pin = GPIO_PIN_11;
      GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
      HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
      GPIO_InitStruct.Pin = GPIO_PIN_12;
      #else
      GPIO_InitStruct.Alternate = GPIO_AF9_CAN1;
      GPIO_InitStruct.Pin = GPIO_PIN_11|GPIO_PIN_12;
      GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
      #endif
      GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
      HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }

     if (_pins == ALT_2)
     {
      __HAL_RCC_GPIOD_CLK_ENABLE();
      GPIO_InitStruct.Pull = GPIO_NOPULL;
      #if defined(__HAL_RCC_AFIO_CLK_ENABLE)
      __HAL_AFIO_REMAP_CAN1_3(); // To use PD0/1 pins for CAN1.
      __HAL_RCC_AFIO_CLK_ENABLE();
      GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
      GPIO_InitStruct.Pin = GPIO_PIN_0;
      GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
      HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
      GPIO_InitStruct.Pin = GPIO_PIN_1;
      #else
      GPIO_InitStruct.Alternate = GPIO_AF9_CAN1;
      GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1;
      GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
      #endif
      GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
      HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
    }

    // NVIC configuration for CAN1 Reception complete interrupt
    HAL_NVIC_SetPriority(CAN1_RX0_IRQn, 5, 0); // 15 is lowest possible priority
    HAL_NVIC_EnableIRQ(CAN1_RX0_IRQn );
    // NVIC configuration for CAN1 Transmission complete interrupt
    HAL_NVIC_SetPriority(CAN1_TX_IRQn, 5, 0); // 15 is lowest possible priority
    HAL_NVIC_EnableIRQ(CAN1_TX_IRQn);

    n_pCanHandle->Instance = CAN1;
  }
#ifdef CAN2
  else if (_canPort == CAN2)
  {
    //CAN2
    __HAL_RCC_CAN1_CLK_ENABLE(); // CAN1 clock needs to be enabled too, because CAN2 works as CAN1 slave.
    __HAL_RCC_CAN2_CLK_ENABLE();
    if (_pins == ALT)
    {
      __HAL_RCC_GPIOB_CLK_ENABLE();
      #if defined(__HAL_RCC_AFIO_CLK_ENABLE)
      __HAL_AFIO_REMAP_CAN2_ENABLE(); // To use PB5/6 pins for CAN2. Don't ask me why this has different name than for CAN1.
      __HAL_RCC_AFIO_CLK_ENABLE();
      GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
      GPIO_InitStruct.Pin = GPIO_PIN_5;
      GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
      HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
      GPIO_InitStruct.Pin = GPIO_PIN_6;
      #else
      GPIO_InitStruct.Alternate = GPIO_AF9_CAN2;
      GPIO_InitStruct.Pin = GPIO_PIN_5|GPIO_PIN_6;
      GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
      #endif
      GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
      HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    }
    if (_pins == DEF) {
      __HAL_RCC_GPIOB_CLK_ENABLE();
      #if defined(__HAL_RCC_AFIO_CLK_ENABLE)
      __HAL_AFIO_REMAP_CAN2_DISABLE(); // To use PB12/13 pins for CAN2.
      __HAL_RCC_AFIO_CLK_ENABLE();
      GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
      GPIO_InitStruct.Pin = GPIO_PIN_12;
      GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
      HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
      GPIO_InitStruct.Pin = GPIO_PIN_13;
      #else
      GPIO_InitStruct.Alternate = GPIO_AF9_CAN2;
      GPIO_InitStruct.Pin = GPIO_PIN_12|GPIO_PIN_13;
      GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
      #endif
      GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
      HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    }

    // NVIC configuration for CAN2 Reception complete interrupt
    HAL_NVIC_SetPriority(CAN2_RX0_IRQn, 5, 0); // 15 is lowest possible priority
    HAL_NVIC_SetPriority(CAN2_RX1_IRQn, 5, 0); // 15 is lowest possible priority
    HAL_NVIC_EnableIRQ(CAN2_RX0_IRQn );
    HAL_NVIC_DisableIRQ(CAN2_RX1_IRQn );
    // NVIC configuration for CAN2 Transmission complete interrupt
    HAL_NVIC_SetPriority(CAN2_TX_IRQn, 5, 0); // 15 is lowest possible priority
    HAL_NVIC_EnableIRQ(CAN2_TX_IRQn);

    n_pCanHandle->Instance = CAN2;
  }
#endif

#ifdef CAN3
  else if (_canPort == CAN3)
  {
    //CAN3
    __HAL_RCC_CAN3_CLK_ENABLE();
    if (_pins == ALT)
    {
      __HAL_RCC_GPIOB_CLK_ENABLE();
      GPIO_InitStruct.Alternate = GPIO_AF11_CAN3;
      GPIO_InitStruct.Pin = GPIO_PIN_3|GPIO_PIN_4;
      GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
      GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
      HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    }
    if (_pins == DEF)
    {
      __HAL_RCC_GPIOA_CLK_ENABLE();
      GPIO_InitStruct.Alternate = GPIO_AF11_CAN3;
      GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_15;
      GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
      GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
      HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }

    // NVIC configuration for CAN3 Reception complete interrupt
    HAL_NVIC_SetPriority(CAN3_RX0_IRQn, 5, 0); // 15 is lowest possible priority
    HAL_NVIC_EnableIRQ(CAN3_RX0_IRQn );
    // NVIC configuration for CAN3 Transmission complete interrupt
    HAL_NVIC_SetPriority(CAN3_TX_IRQn, 5, 0); // 15 is lowest possible priority
    HAL_NVIC_EnableIRQ(CAN3_TX_IRQn);

    n_pCanHandle->Instance = CAN3;
  }
#endif

  n_pCanHandle->Init.TimeTriggeredMode = DISABLE;
  n_pCanHandle->Init.AutoBusOff = ENABLE;
  n_pCanHandle->Init.AutoWakeUp = DISABLE;
  if (retransmission){ n_pCanHandle->Init.AutoRetransmission  = ENABLE; }
  else { n_pCanHandle->Init.AutoRetransmission  = DISABLE; }
  n_pCanHandle->Init.ReceiveFifoLocked  = DISABLE;
  n_pCanHandle->Init.TransmitFifoPriority = DISABLE;
  n_pCanHandle->Init.Mode = CAN_MODE_NORMAL;
}

void STM32_CAN::setBaudRate(uint32_t baud)
{

  // Calculate and set baudrate
  calculateBaudrate( n_pCanHandle, baud );

  // Initializes CAN
  HAL_CAN_Init( n_pCanHandle );

  initializeFilters();

  // Start the CAN peripheral
  HAL_CAN_Start( n_pCanHandle );

  // Activate CAN RX notification
  HAL_CAN_ActivateNotification( n_pCanHandle, CAN_IT_RX_FIFO0_MSG_PENDING);

  // Activate CAN TX notification
  HAL_CAN_ActivateNotification( n_pCanHandle, CAN_IT_TX_MAILBOX_EMPTY);
}

bool STM32_CAN::write(CAN_message_t &CAN_tx_msg, bool sendMB)
{
  bool ret = true;
  uint32_t TxMailbox;
  CAN_TxHeaderTypeDef TxHeader;

  __HAL_CAN_DISABLE_IT(n_pCanHandle, CAN_IT_TX_MAILBOX_EMPTY);

  if (CAN_tx_msg.flags.extended == 1) // Extended ID when CAN_tx_msg.flags.extended is 1
  {
      TxHeader.ExtId = CAN_tx_msg.id;
      TxHeader.IDE   = CAN_ID_EXT;
  }
  else // Standard ID otherwise
  {
      TxHeader.StdId = CAN_tx_msg.id;
      TxHeader.IDE   = CAN_ID_STD;
  }

  TxHeader.RTR   = CAN_RTR_DATA;
  TxHeader.DLC   = CAN_tx_msg.len;
  TxHeader.TransmitGlobalTime = DISABLE;

  if(HAL_CAN_AddTxMessage( n_pCanHandle, &TxHeader, CAN_tx_msg.buf, &TxMailbox) != HAL_OK)
  {
    /* in normal situation we add up the message to TX ring buffer, if there is no free TX mailbox. But the TX mailbox interrupt is using this same function
    to move the messages from ring buffer to empty TX mailboxes, so for that use case, there is this check */
    if(sendMB != true)
    {
      if( addToRingBuffer(txRing, CAN_tx_msg) == false )
      {
        ret = false; // no more room
      }
    }
    else { ret = false; }
  }
  __HAL_CAN_ENABLE_IT(n_pCanHandle, CAN_IT_TX_MAILBOX_EMPTY);

  return ret;
}

bool STM32_CAN::read(CAN_message_t &CAN_rx_msg)
{
  bool ret;
  __HAL_CAN_DISABLE_IT(n_pCanHandle, CAN_IT_RX_FIFO0_MSG_PENDING);
  ret = removeFromRingBuffer(rxRing, CAN_rx_msg);
  __HAL_CAN_ENABLE_IT(n_pCanHandle, CAN_IT_RX_FIFO0_MSG_PENDING);
  return ret;
}

bool STM32_CAN::setFilter(uint8_t bank_num, uint32_t filter_id, uint32_t mask, uint32_t filter_mode, uint32_t filter_scale, uint32_t fifo)
{
  CAN_FilterTypeDef sFilterConfig;

  sFilterConfig.FilterBank = bank_num;
  sFilterConfig.FilterMode = filter_mode;
  sFilterConfig.FilterScale = filter_scale;
  sFilterConfig.FilterFIFOAssignment = fifo;
  sFilterConfig.FilterActivation = ENABLE;

  if (filter_id <= 0x7FF)
  {
    // Standard ID can be only 11 bits long
    sFilterConfig.FilterIdHigh = (uint16_t) (filter_id << 5);
    sFilterConfig.FilterIdLow = 0;
    sFilterConfig.FilterMaskIdHigh = (uint16_t) (mask << 5);
    sFilterConfig.FilterMaskIdLow = CAN_ID_EXT;
  }
  else
  {
    // Extended ID
    sFilterConfig.FilterIdLow = (uint16_t) (filter_id << 3);
    sFilterConfig.FilterIdLow |= CAN_ID_EXT;
    sFilterConfig.FilterIdHigh = (uint16_t) (filter_id >> 13);
    sFilterConfig.FilterMaskIdLow = (uint16_t) (mask << 3);
    sFilterConfig.FilterMaskIdLow |= CAN_ID_EXT;
    sFilterConfig.FilterMaskIdHigh = (uint16_t) (mask >> 13);
  }

  // Enable filter
  if (HAL_CAN_ConfigFilter( n_pCanHandle, &sFilterConfig ) != HAL_OK)
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

void STM32_CAN::setMBFilter(CAN_BANK bank_num, CAN_FLTEN input)
{
  CAN_FilterTypeDef sFilterConfig;
  sFilterConfig.FilterBank = uint8_t(bank_num);
  if (input == ACCEPT_ALL) { sFilterConfig.FilterActivation = ENABLE; }
  else { sFilterConfig.FilterActivation = DISABLE; }

  HAL_CAN_ConfigFilter(n_pCanHandle, &sFilterConfig);
}

void STM32_CAN::setMBFilter(CAN_FLTEN input)
{
  CAN_FilterTypeDef sFilterConfig;
  uint8_t max_bank_num = 27;
  uint8_t min_bank_num = 0;
  #ifdef CAN2
  if (_canPort == CAN1){ max_bank_num = 13;}
  else if (_canPort == CAN2){ min_bank_num = 14;}
  #endif
  for (uint8_t bank_num = min_bank_num ; bank_num <= max_bank_num ; bank_num++)
  {
    sFilterConfig.FilterBank = bank_num;
    if (input == ACCEPT_ALL) { sFilterConfig.FilterActivation = ENABLE; }
    else { sFilterConfig.FilterActivation = DISABLE; }
    HAL_CAN_ConfigFilter(n_pCanHandle, &sFilterConfig);
  }
}

bool STM32_CAN::setMBFilterProcessing(CAN_BANK bank_num, uint32_t filter_id, uint32_t mask)
{
  // just convert the MB number enum to bank number.
  return setFilter(uint8_t(bank_num), filter_id, mask);
}

bool STM32_CAN::setMBFilter(CAN_BANK bank_num, uint32_t id1)
{
  // by setting the mask to 0x1FFFFFFF we only filter the ID set as Filter ID.
  return setFilter(uint8_t(bank_num), id1, 0x1FFFFFFF);
}

bool STM32_CAN::setMBFilter(CAN_BANK bank_num, uint32_t id1, uint32_t id2)
{
  // if we set the filter mode as IDLIST, the mask becomes filter ID too. So we can filter two totally independent IDs in same bank.
  return setFilter(uint8_t(bank_num), id1, id2, CAN_FILTERMODE_IDLIST);
}

// TBD, do this using "setFilter" -function
void STM32_CAN::initializeFilters()
{
  CAN_FilterTypeDef sFilterConfig;
  // We set first bank to accept all RX messages
  sFilterConfig.FilterBank = 0;
  sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
  sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
  sFilterConfig.FilterIdHigh = 0x0000;
  sFilterConfig.FilterIdLow = 0x0000;
  sFilterConfig.FilterMaskIdHigh = 0x0000;
  sFilterConfig.FilterMaskIdLow = 0x0000;
  sFilterConfig.FilterFIFOAssignment = CAN_RX_FIFO0;
  sFilterConfig.FilterActivation = ENABLE;
  #ifdef CAN2
  // Filter banks from 14 to 27 are for Can2, so first for Can2 is bank 14. This is not relevant for devices with only one CAN
  if (_canPort == CAN1)
  {
    sFilterConfig.SlaveStartFilterBank = 14;
  }
  if (_canPort == CAN2)
  {
    sFilterConfig.FilterBank = 14;
  }
  #endif

  HAL_CAN_ConfigFilter(n_pCanHandle, &sFilterConfig);
}

void STM32_CAN::initializeBuffers()
{
    if(isInitialized()) { return; }

    // set up the transmit and receive ring buffers
    if(tx_buffer==nullptr)
    {
      tx_buffer=new CAN_message_t[sizeTxBuffer];
    }
    initRingBuffer(txRing, tx_buffer, sizeTxBuffer);

    if(rx_buffer==nullptr)
    {
      rx_buffer=new CAN_message_t[sizeRxBuffer];
    }
    initRingBuffer(rxRing, rx_buffer, sizeRxBuffer);
}

void STM32_CAN::initRingBuffer(RingbufferTypeDef &ring, volatile CAN_message_t *buffer, uint32_t size)
{
    ring.buffer = buffer;
    ring.size = size;
    ring.head = 0;
    ring.tail = 0;
}

bool STM32_CAN::addToRingBuffer(RingbufferTypeDef &ring, const CAN_message_t &msg)
{
    uint16_t nextEntry;
    nextEntry =(ring.head + 1) % ring.size;

    // check if the ring buffer is full
    if(nextEntry == ring.tail)
	{
        return(false);
    }

    // add the element to the ring */
    memcpy((void *)&ring.buffer[ring.head],(void *)&msg, sizeof(CAN_message_t));

    // bump the head to point to the next free entry
    ring.head = nextEntry;

    return(true);
}
bool STM32_CAN::removeFromRingBuffer(RingbufferTypeDef &ring, CAN_message_t &msg)
{
    // check if the ring buffer has data available
    if(isRingBufferEmpty(ring) == true)
    {
        return(false);
    }

    // copy the message
    memcpy((void *)&msg,(void *)&ring.buffer[ring.tail], sizeof(CAN_message_t));

    // bump the tail pointer
    ring.tail =(ring.tail + 1) % ring.size;
    return(true);
}

bool STM32_CAN::isRingBufferEmpty(RingbufferTypeDef &ring)
{
    if(ring.head == ring.tail)
	{
        return(true);
    }

    return(false);
}

uint32_t STM32_CAN::ringBufferCount(RingbufferTypeDef &ring)
{
    int32_t entries;
    entries = ring.head - ring.tail;

    if(entries < 0)
    {
        entries += ring.size;
    }
    return((uint32_t)entries);
}

void STM32_CAN::setBaudRateValues(CAN_HandleTypeDef *CanHandle, uint16_t prescaler, uint8_t timeseg1,
                                                                uint8_t timeseg2, uint8_t sjw)
{
  uint32_t _SyncJumpWidth = 0;
  uint32_t _TimeSeg1 = 0;
  uint32_t _TimeSeg2 = 0;
  uint32_t _Prescaler = 0;
  switch (sjw)
  {
    case 1:
      _SyncJumpWidth = CAN_SJW_1TQ;
      break;
    case 2:
      _SyncJumpWidth = CAN_SJW_2TQ;
      break;
    case 3:
      _SyncJumpWidth = CAN_SJW_3TQ;
      break;
    case 4:
      _SyncJumpWidth = CAN_SJW_4TQ;
      break;
    default:
      // should not happen
      _SyncJumpWidth = CAN_SJW_1TQ;
      break;
  }

  switch (timeseg1)
  {
    case 1:
      _TimeSeg1 = CAN_BS1_1TQ;
      break;
    case 2:
      _TimeSeg1 = CAN_BS1_2TQ;
      break;
    case 3:
      _TimeSeg1 = CAN_BS1_3TQ;
      break;
    case 4:
      _TimeSeg1 = CAN_BS1_4TQ;
      break;
    case 5:
      _TimeSeg1 = CAN_BS1_5TQ;
      break;
    case 6:
      _TimeSeg1 = CAN_BS1_6TQ;
      break;
    case 7:
      _TimeSeg1 = CAN_BS1_7TQ;
      break;
    case 8:
      _TimeSeg1 = CAN_BS1_8TQ;
      break;
    case 9:
      _TimeSeg1 = CAN_BS1_9TQ;
      break;
    case 10:
      _TimeSeg1 = CAN_BS1_10TQ;
      break;
    case 11:
      _TimeSeg1 = CAN_BS1_11TQ;
      break;
    case 12:
      _TimeSeg1 = CAN_BS1_12TQ;
      break;
    case 13:
      _TimeSeg1 = CAN_BS1_13TQ;
      break;
    case 14:
      _TimeSeg1 = CAN_BS1_14TQ;
      break;
    case 15:
      _TimeSeg1 = CAN_BS1_15TQ;
      break;
    case 16:
      _TimeSeg1 = CAN_BS1_16TQ;
      break;
    default:
      // should not happen
      _TimeSeg1 = CAN_BS1_1TQ;
      break;
  }

  switch (timeseg2)
  {
    case 1:
      _TimeSeg2 = CAN_BS2_1TQ;
      break;
    case 2:
      _TimeSeg2 = CAN_BS2_2TQ;
      break;
    case 3:
      _TimeSeg2 = CAN_BS2_3TQ;
      break;
    case 4:
      _TimeSeg2 = CAN_BS2_4TQ;
      break;
    case 5:
      _TimeSeg2 = CAN_BS2_5TQ;
      break;
    case 6:
      _TimeSeg2 = CAN_BS2_6TQ;
      break;
    case 7:
      _TimeSeg2 = CAN_BS2_7TQ;
      break;
    case 8:
      _TimeSeg2 = CAN_BS2_8TQ;
      break;
    default:
      // should not happen
      _TimeSeg2 = CAN_BS2_1TQ;
      break;
  }
  _Prescaler = prescaler;

  CanHandle->Init.SyncJumpWidth = _SyncJumpWidth;
  CanHandle->Init.TimeSeg1 = _TimeSeg1;
  CanHandle->Init.TimeSeg2 = _TimeSeg2;
  CanHandle->Init.Prescaler = _Prescaler;
}

void STM32_CAN::calculateBaudrate(CAN_HandleTypeDef *CanHandle, int baud)
{
  /* this function calculates the needed Sync Jump Width, Time segments 1 and 2 and prescaler values based on the set baud rate and APB1 clock.
  This could be done faster if needed by calculating these values beforehand and just using fixed values from table.
  The function has been optimized to give values that have sample-point between 75-94%. If some other sample-point percentage is needed, this needs to be adjusted.
  More info about this topic here: http://www.bittiming.can-wiki.info/
  */
  int sjw = 1;
  int bs1 = 5; // optimization. bs1 smaller than 5 does give too small sample-point percentages.
  int bs2 = 1;
  int prescaler = 1;
  uint16_t i = 0;

  uint32_t frequency = getAPB1Clock();

  if(frequency == 48000000) {
    for(i=0; i<sizeof(BAUD_RATE_TABLE_48M)/sizeof(Baudrate_entry_t); i++) {
      if(baud == (int)BAUD_RATE_TABLE_48M[i].baudrate) {
        break;
      }
    }
    if(i < sizeof(BAUD_RATE_TABLE_48M)/sizeof(Baudrate_entry_t)) {
      setBaudRateValues(CanHandle, BAUD_RATE_TABLE_48M[i].prescaler,
                                   BAUD_RATE_TABLE_48M[i].timeseg1,
                                   BAUD_RATE_TABLE_48M[i].timeseg2,
                                   1);
      return;
    }
  }
  else if(frequency == 45000000) {
    for(i=0; i<sizeof(BAUD_RATE_TABLE_45M)/sizeof(Baudrate_entry_t); i++) {
      if(baud == (int)BAUD_RATE_TABLE_45M[i].baudrate) {
        break;
      }
    }
    if(i < sizeof(BAUD_RATE_TABLE_45M)/sizeof(Baudrate_entry_t)) {
      setBaudRateValues(CanHandle, BAUD_RATE_TABLE_45M[i].prescaler,
                                   BAUD_RATE_TABLE_45M[i].timeseg1,
                                   BAUD_RATE_TABLE_45M[i].timeseg2,
                                   1);
      return;
    }
  }
  else if(frequency == 42000000) {
    for(i=0; i<sizeof(BAUD_RATE_TABLE_42M)/sizeof(Baudrate_entry_t); i++) {
      if(baud == (int)BAUD_RATE_TABLE_42M[i].baudrate) {
        break;
      }
    }
    if(i < sizeof(BAUD_RATE_TABLE_42M)/sizeof(Baudrate_entry_t)) {
      setBaudRateValues(CanHandle, BAUD_RATE_TABLE_42M[i].prescaler,
                                   BAUD_RATE_TABLE_42M[i].timeseg1,
                                   BAUD_RATE_TABLE_42M[i].timeseg2,
                                   1);
      return;
    }
  }

  while (sjw <= 4) {
    while (prescaler <= 1024) {
      while (bs2 <= 3) { // Time segment 2 can get up to 8, but that causes too small sample-point percentages, so this is limited to 3.
        while (bs1 <= 15) { // Time segment 1 can get up to 16, but that causes too big sample-point percenages, so this is limited to 15.
          int calcBaudrate = (int)(frequency / (prescaler * (sjw + bs1 + bs2)));

          if (calcBaudrate == baud)
          {
            setBaudRateValues(CanHandle, prescaler, bs1, bs2, sjw);
            return;
          }
          bs1++;
        }
        bs1 = 5;
        bs2++;
      }
      bs1 = 5;
      bs2 = 1;
      prescaler++;
    }
    bs1 = 5;
    sjw++;
  }
}

uint32_t STM32_CAN::getAPB1Clock()
{
  RCC_ClkInitTypeDef clkInit;
  uint32_t flashLatency;
  HAL_RCC_GetClockConfig(&clkInit, &flashLatency);

  uint32_t hclkClock = HAL_RCC_GetHCLKFreq();
  uint8_t clockDivider = 1;
  switch (clkInit.APB1CLKDivider)
  {
  case RCC_HCLK_DIV1:
    clockDivider = 1;
    break;
  case RCC_HCLK_DIV2:
    clockDivider = 2;
    break;
  case RCC_HCLK_DIV4:
    clockDivider = 4;
    break;
  case RCC_HCLK_DIV8:
    clockDivider = 8;
    break;
  case RCC_HCLK_DIV16:
    clockDivider = 16;
    break;
  default:
    // should not happen
    break;
  }

  uint32_t apb1Clock = hclkClock / clockDivider;

  return apb1Clock;
}

void STM32_CAN::enableMBInterrupts()
{
  if (n_pCanHandle->Instance == CAN1)
  {
    HAL_NVIC_EnableIRQ(CAN1_TX_IRQn);
  }
#ifdef CAN2
  else if (n_pCanHandle->Instance == CAN2)
  {
    HAL_NVIC_EnableIRQ(CAN2_TX_IRQn);
  }
#endif
#ifdef CAN3
  else if (n_pCanHandle->Instance == CAN3)
  {
    HAL_NVIC_EnableIRQ(CAN3_TX_IRQn);
  }
#endif
}

void STM32_CAN::disableMBInterrupts()
{
  if (n_pCanHandle->Instance == CAN1)
  {
    HAL_NVIC_DisableIRQ(CAN1_TX_IRQn);
  }
#ifdef CAN2
  else if (n_pCanHandle->Instance == CAN2)
  {
    HAL_NVIC_DisableIRQ(CAN2_TX_IRQn);
  }
#endif
#ifdef CAN3
  else if (n_pCanHandle->Instance == CAN3)
  {
    HAL_NVIC_DisableIRQ(CAN3_TX_IRQn);
  }
#endif
}

void STM32_CAN::enableLoopBack( bool yes ) {
  if (yes) { n_pCanHandle->Init.Mode = CAN_MODE_LOOPBACK; }
  else { n_pCanHandle->Init.Mode = CAN_MODE_NORMAL; }
}

void STM32_CAN::enableSilentMode( bool yes ) {
  if (yes) { n_pCanHandle->Init.Mode = CAN_MODE_SILENT; }
  else { n_pCanHandle->Init.Mode = CAN_MODE_NORMAL; }
}

void STM32_CAN::enableSilentLoopBack( bool yes ) {
  if (yes) { n_pCanHandle->Init.Mode = CAN_MODE_SILENT_LOOPBACK; }
  else { n_pCanHandle->Init.Mode = CAN_MODE_NORMAL; }
}

void STM32_CAN::enableFIFO(bool status)
{
  //Nothing to do here. The FIFO is on by default. This is just to work with code made for Teensy FlexCan.
}

uint32_t STM32_CAN::getError(){
  uint32_t error = n_pCanHandle->ErrorCode;
  HAL_CAN_ResetError(n_pCanHandle);
  return error;
}

uint8_t STM32_CAN::getREC() {
  return (uint8_t)((n_pCanHandle->Instance->ESR & CAN_ESR_REC) >> CAN_ESR_REC_Pos);
}

uint8_t STM32_CAN::getTEC() {
  return (uint8_t)((n_pCanHandle->Instance->ESR & CAN_ESR_TEC) >> CAN_ESR_TEC_Pos);
}

STM32_CAN::BusState_t STM32_CAN::getBusState() {
  if((n_pCanHandle->Instance->ESR & CAN_ESR_BOFF) != 0) {
    return BUS_OFF;
  } else if((n_pCanHandle->Instance->ESR & CAN_ESR_EPVF) != 0) {
    return BUS_ERROR;
  } else if((n_pCanHandle->Instance->ESR & CAN_ESR_EWGF) != 0) {
    return BUS_WARNING;
  } else {
    return BUS_NORMAL;
  }
}

/* Interrupt functions
-----------------------------------------------------------------------------------------------------------------------------------------------------------------
*/

/* ==== HAL CAN Errors ====
HAL_CAN_ERROR_NONE            (0x00000000U)  //!< No error
HAL_CAN_ERROR_EWG             (0x00000001U)  //!< Protocol Error Warning
HAL_CAN_ERROR_EPV             (0x00000002U)  //!< Error Passive
HAL_CAN_ERROR_BOF             (0x00000004U)  //!< Bus-off error
HAL_CAN_ERROR_STF             (0x00000008U)  //!< Stuff error
HAL_CAN_ERROR_FOR             (0x00000010U)  //!< Form error
HAL_CAN_ERROR_ACK             (0x00000020U)  //!< Acknowledgment error
HAL_CAN_ERROR_BR              (0x00000040U)  //!< Bit recessive error
HAL_CAN_ERROR_BD              (0x00000080U)  //!< Bit dominant error
HAL_CAN_ERROR_CRC             (0x00000100U)  //!< CRC error
HAL_CAN_ERROR_RX_FOV0         (0x00000200U)  //!< Rx FIFO0 overrun error
HAL_CAN_ERROR_RX_FOV1         (0x00000400U)  //!< Rx FIFO1 overrun error
HAL_CAN_ERROR_TX_ALST0        (0x00000800U)  //!< TxMailbox 0 transmit failure due to arbitration lost
HAL_CAN_ERROR_TX_TERR0        (0x00001000U)  //!< TxMailbox 0 transmit failure due to transmit error
HAL_CAN_ERROR_TX_ALST1        (0x00002000U)  //!< TxMailbox 1 transmit failure due to arbitration lost
HAL_CAN_ERROR_TX_TERR1        (0x00004000U)  //!< TxMailbox 1 transmit failure due to transmit error
HAL_CAN_ERROR_TX_ALST2        (0x00008000U)  //!< TxMailbox 2 transmit failure due to arbitration lost
HAL_CAN_ERROR_TX_TERR2        (0x00010000U)  //!< TxMailbox 2 transmit failure due to transmit error
HAL_CAN_ERROR_TIMEOUT         (0x00020000U)  //!< Timeout error
HAL_CAN_ERROR_NOT_INITIALIZED (0x00040000U)  //!< Peripheral not initialized
HAL_CAN_ERROR_NOT_READY       (0x00080000U)  //!< Peripheral not ready
HAL_CAN_ERROR_NOT_STARTED     (0x00100000U)  //!< Peripheral not started
HAL_CAN_ERROR_PARAM           (0x00200000U)  //!< Parameter error
HAL_CAN_ERROR_INVALID_CALLBACK (0x00400000U) /*!< Invalid Callback error
HAL_CAN_ERROR_INTERNAL        (0x00800000U)  /*!< Internal error
*/


// There is 3 TX mailboxes. Each one has own transmit complete callback function, that we use to pull next message from TX ringbuffer to be sent out in TX mailbox.
extern "C" void HAL_CAN_TxMailbox0CompleteCallback( CAN_HandleTypeDef *CanHandle )
{
  CAN_message_t txmsg;
  // use correct CAN instance
  if (CanHandle->Instance == CAN1)
  {
    if (_CAN1->removeFromRingBuffer(_CAN1->txRing, txmsg))
    {
      _CAN1->write(txmsg, true);
    }
  }
#ifdef CAN2
  else if (CanHandle->Instance == CAN2)
  {
    if (_CAN2->removeFromRingBuffer(_CAN2->txRing, txmsg))
    {
      _CAN2->write(txmsg, true);
    }
  }
#endif
#ifdef CAN3
  else if (CanHandle->Instance == CAN3)
  {
    if (_CAN3->removeFromRingBuffer(_CAN3->txRing, txmsg))
    {
      _CAN3->write(txmsg, true);
    }
  }
#endif
}

extern "C" void HAL_CAN_TxMailbox1CompleteCallback( CAN_HandleTypeDef *CanHandle )
{
  CAN_message_t txmsg;
  // use correct CAN instance
  if (CanHandle->Instance == CAN1)
  {
    if (_CAN1->removeFromRingBuffer(_CAN1->txRing, txmsg))
    {
      _CAN1->write(txmsg, true);
    }
  }
#ifdef CAN2
  else if (CanHandle->Instance == CAN2)
  {
    if (_CAN2->removeFromRingBuffer(_CAN2->txRing, txmsg))
    {
      _CAN2->write(txmsg, true);
    }
  }
#endif
#ifdef CAN3
  else if (CanHandle->Instance == CAN3)
  {
    if (_CAN3->removeFromRingBuffer(_CAN3->txRing, txmsg))
    {
      _CAN3->write(txmsg, true);
    }
  }
#endif
}

extern "C" void HAL_CAN_TxMailbox2CompleteCallback( CAN_HandleTypeDef *CanHandle )
{
  CAN_message_t txmsg;
  // use correct CAN instance
  if (CanHandle->Instance == CAN1)
  {
    if (_CAN1->removeFromRingBuffer(_CAN1->txRing, txmsg))
    {
      _CAN1->write(txmsg, true);
    }
  }
#ifdef CAN2
  else if (CanHandle->Instance == CAN2)
  {
    if (_CAN2->removeFromRingBuffer(_CAN2->txRing, txmsg))
    {
      _CAN2->write(txmsg, true);
    }
  }
#endif
#ifdef CAN3
  else if (CanHandle->Instance == CAN3)
  {
    if (_CAN3->removeFromRingBuffer(_CAN3->txRing, txmsg))
    {
      _CAN3->write(txmsg, true);
    }
  }
#endif
}

// This is called by RX0_IRQHandler when there is message at RX FIFO0 buffer
extern "C" void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *CanHandle)
{
  CAN_message_t rxmsg;
  CAN_RxHeaderTypeDef   RxHeader;
  //bool state = Disable_Interrupts();

  // move the message from RX FIFO0 to RX ringbuffer
  if (HAL_CAN_GetRxMessage( CanHandle, CAN_RX_FIFO0, &RxHeader, rxmsg.buf ) == HAL_OK)
  {
    if ( RxHeader.IDE == CAN_ID_STD )
    {
      rxmsg.id = RxHeader.StdId;
      rxmsg.flags.extended = 0;
    }
    else
    {
      rxmsg.id = RxHeader.ExtId;
      rxmsg.flags.extended = 1;
    }

    rxmsg.flags.remote = RxHeader.RTR;
    rxmsg.mb           = RxHeader.FilterMatchIndex;
    rxmsg.timestamp    = RxHeader.Timestamp;
    rxmsg.len          = RxHeader.DLC;

    // use correct ring buffer based on CAN instance
    if (CanHandle->Instance == CAN1)
    {
      rxmsg.bus = 1;
      _CAN1->addToRingBuffer(_CAN1->rxRing, rxmsg);
    }
#ifdef CAN2
    else if (CanHandle->Instance == CAN2)
    {
      rxmsg.bus = 2;
      _CAN2->addToRingBuffer(_CAN2->rxRing, rxmsg);
    }
#endif
#ifdef CAN3
    else if (CanHandle->Instance == CAN3)
    {
      rxmsg.bus = 3;
      _CAN3->addToRingBuffer(_CAN3->rxRing, rxmsg);
    }
#endif
  }
  //Enable_Interrupts(state);
}

// RX IRQ handlers
extern "C" void CAN1_RX0_IRQHandler(void)
{
  HAL_CAN_IRQHandler(&hcan1);
}

#ifdef CAN2
extern "C" void CAN2_RX0_IRQHandler(void)
{
  HAL_CAN_IRQHandler(&hcan2);
}
#endif
#ifdef CAN3
extern "C" void CAN3_RX0_IRQHandler(void)
{
  HAL_CAN_IRQHandler(&hcan3);
}
#endif

// TX IRQ handlers
extern "C" void CAN1_TX_IRQHandler(void)
{
  HAL_CAN_IRQHandler(&hcan1);
}

#ifdef CAN2
extern "C" void CAN2_TX_IRQHandler(void)
{
  HAL_CAN_IRQHandler(&hcan2);
}
#endif
#ifdef CAN3
extern "C" void CAN3_TX_IRQHandler(void)
{
  HAL_CAN_IRQHandler(&hcan3);
}
#endif
