#include <board.h>
#include <aic.h>
#include <pio.h>
#include <spi.h>

static 	AT91S_SPI *spi = AT91C_BASE_SPI0;

static const Pin PIN_spi_cs =   {1 << 12, AT91C_BASE_PIOA, AT91C_ID_PIOA, PIO_PERIPH_A, PIO_DEFAULT};
static const Pin PIN_spi_miso = {1 << 16, AT91C_BASE_PIOA, AT91C_ID_PIOA, PIO_PERIPH_A, PIO_DEFAULT};
static const Pin PIN_spi_mosi = {1 << 17, AT91C_BASE_PIOA, AT91C_ID_PIOA, PIO_PERIPH_A, PIO_DEFAULT};
static const Pin PIN_spi_sck =  {1 << 18, AT91C_BASE_PIOA, AT91C_ID_PIOA, PIO_PERIPH_A, PIO_DEFAULT};


#define EOT 0x100
#define RX_BUF_SIZE 1024
#define TX_BUF_SIZE 32
#define RX_BUF_SIZE_MASK 0x3ff

static volatile unsigned short spi_rx_buf[RX_BUF_SIZE];
static volatile unsigned char spi_tx_buf[TX_BUF_SIZE];
static volatile int tx_buf_rdpos;
static volatile int tx_buf_count;

static volatile int rx_buf_count;
static volatile int rx_buf_wrpos, rx_buf_rdpos;
static volatile int rx_packets = 0;

void spi_irq()
{
	volatile int status = spi->SPI_SR;

	if(status & AT91C_SPI_RDRF)
	{
		unsigned char val = spi->SPI_RDR;

//			printf("SP %x", val);

			if(rx_buf_count < RX_BUF_SIZE-1)
			{
				spi_rx_buf[rx_buf_wrpos++] = val;
				rx_buf_wrpos &= RX_BUF_SIZE_MASK;
			}

			if(tx_buf_count > 0 && tx_buf_rdpos < TX_BUF_SIZE)
			{
				unsigned char tdr= spi_tx_buf[tx_buf_rdpos++];
//				printf("T %x c %d rp %d", tdr, tx_buf_count, tx_buf_rdpos);
				spi->SPI_TDR = tdr;
				spi->SPI_TDR = tdr;
				tx_buf_count--;
			}
	}

	if (status & AT91C_SPI_OVRES)
	{
		volatile unsigned char val = spi->SPI_RDR;
	}

	if (status & AT91C_SPI_NSSR)
	{
		if(rx_buf_count < RX_BUF_SIZE)
		{
			spi_rx_buf[rx_buf_wrpos++] = EOT;
			rx_buf_wrpos &= RX_BUF_SIZE_MASK;
		}

	//	printf("NSSR");

		rx_packets++;
	}

}

int spi_slave_poll()
{
	return (rx_packets > 0);
}

int spi_slave_read(unsigned char *buf, int max_len)
{
	int cnt ;
	if(rx_packets)
	{
		unsigned short val;
		int i = 0;

		while((val = spi_rx_buf[rx_buf_rdpos++]) != EOT)
		{
			if(i<max_len)
				buf[i] =  val & 0xff;

			i++;
			rx_buf_rdpos &= RX_BUF_SIZE_MASK;
		}
		rx_buf_rdpos &= RX_BUF_SIZE_MASK;

		AIC_DisableIT(AT91C_ID_SPI0);
		rx_buf_count -= i + 1;
		rx_packets--;
		AIC_EnableIT(AT91C_ID_SPI0);
		return i;
	}
	return 0;
}

int spi_slave_write(unsigned char *buf, int len)
{
		AIC_DisableIT(AT91C_ID_SPI0);
		memcpy(spi_tx_buf,buf ,len);
		spi->SPI_TDR=buf[0];
		tx_buf_rdpos = 1;
		tx_buf_count = len-1;
		AIC_EnableIT(AT91C_ID_SPI0);

}

void spi_slave_init()
{
	PMC_EnablePeripheral(AT91C_ID_SPI0);
	PMC_EnablePeripheral(AT91C_ID_PIOA);

	PIO_Configure(&PIN_spi_cs, 1);
	PIO_Configure(&PIN_spi_miso, 1);
	PIO_Configure(&PIN_spi_mosi, 1);
	PIO_Configure(&PIN_spi_sck, 1);

	spi->SPI_CR = AT91C_SPI_SWRST | AT91C_SPI_SPIEN;
	spi->SPI_CR = AT91C_SPI_SPIEN;
	spi->SPI_CSR[0] = 0;
	spi->SPI_IER = AT91C_SPI_NSSR | AT91C_SPI_RDRF | AT91C_SPI_OVRES;
	spi->SPI_MR = 0; // slave mode

	rx_buf_rdpos = 0;
	rx_buf_wrpos = 0;
	rx_buf_count = 0;

	rx_packets = 0;
	tx_buf_count = 0;
	tx_buf_rdpos = 0;

	AIC_ConfigureIT(AT91C_ID_SPI0,AT91C_AIC_SRCTYPE_INT_HIGH_LEVEL, spi_irq);
	AIC_EnableIT(AT91C_ID_SPI0);

}