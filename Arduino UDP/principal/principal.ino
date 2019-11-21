#include <SPI.h>
#include <Ethernet.h>
#include <EEPROM.h>
#include <utility/w5100.h>
/*
SECTORES DE MEMORIA EEPROM
*/
#define IPEEPROM  0x0000
#define NETMASKEEPROM  0x0004
#define GATEWAYEEPROM  0x0008
#define DNSSEEPROM  0x000C
#define IPSERVIDOREEPROM  0x0010
#define DHCPSTATUSEEPROM  0x0014
#define YAESCRITO         0x0015///Si es 1 ya esta escrito si es cualquier Valor no Esta Escrito

#define textBuffSize 12 //length of longest command string plus two spaces for CR + LF
/*
VARIABLES GLOBALES
*/
byte mac1[] = { 0x90, 0xA2, 0xDA, 0x0F, 0xD3, 0x3B };
IPAddress ipl(192, 168, 10, 172);
IPAddress dnsl(192, 168, 10, 1);
IPAddress gatewayl(192, 168, 10, 1);
IPAddress subnetl(255, 255, 255, 0);
IPAddress servidor1(192,168,10,20);
int puerto = 1337;
char dhcpopcion = 0;

EthernetClient Cliente ;//ojo con igualar a a 0 asi no funciona error de arduino
EthernetClient Clientebk = Cliente;

EthernetClient ClienteT ;//ojo con igualar a a 0 asi no funciona error de arduino

EthernetServer ServidorT(23);


char textBuff[textBuffSize] = { 0x00 }; //someplace to put received text
int charsReceived = 0;
char destino;
unsigned long timeOfLastActivity; //time in milliseconds of last activity
unsigned long allowedConnectTime = 300000; //five minutes
boolean alreadyConnected = false; // whether or not the client was connected previously
int k;
char banderareset = 0;
//Errores
int error, z1, z2 = 0;
//Mensaje de Salida
char msjout[20] = "Soy Erick Arzola!";
//Mensaje de Entrada
char msjin[30] = "";

int connect_to_Server(IPAddress servidor, int puerto, int timeout);
int disconnect_to_Server();
int write_to_server(char * msjout);
int read_from_Server(char * msjin);
int status_Server();
int Convertidor();
void capturaTelnet( char* instruccion);
void telnet();
int checkConnectionTimeout();
int recuperaDatos();
int salvaDatos();
void softReset();
void closer(SOCKET s);
void closeSockets();

void setup()
{
	Serial.begin(9600);
	while (!Serial) {
		; // wait for serial port to connect. Needed for native USB port only
        }
        //Lee Datos de la EEPROM
        recuperaDatos();
        
        // initialize the ethernet device
	Ethernet.begin(mac1, ipl, dnsl, gatewayl, subnetl);
        ServidorT.begin();
	// Open serial communications and wait for port to open:
	Serial.print("Direccion IP Cliente / Telnet: ");
	Serial.println(Ethernet.localIP());
	Serial.print("Gateway Cliente: ");
	Serial.println(Ethernet.gatewayIP());
	Serial.print("DNS Cliente: ");
	Serial.println(Ethernet.dnsServerIP());
	Serial.print("Mascara de Red del Cliente: ");
	Serial.println(Ethernet.subnetMask());
        Serial.print("IP Servidor Destino: ");
	Serial.println(servidor1);
}

void loop()
{

  if (ServidorT.available() ) {// REvisa Conexion entrante Telnet
		telnet();
		ClienteT.println("Salida.... Adios Arzola");
                Serial.println("\nSalio Usuario.....");
                ClienteT.stop();
                ServidorT.begin();
                //closer(0x1C);
                if(banderareset == 1){
                    Serial.println("\nREINCIANDO ARDUINO PARA APLICAR LOS CAMBIOS ESPERE.....");
                    _delay_ms(10000);
                    softReset();// aqui en caso de algun cambio se reinciia
                }
	}
  else{//se Conecta a un Servidor Como Cliente
              error = connect_to_Server(servidor1,puerto,10);
              if (error == 1 ) //Significa que si se conecto
		{
  /*
			while (status_Server() == 1)// Este while es para mantener ciclado, se puede omitir 
			{
				z2 = read_from_Server(msjin);		
				if (z2 > 1)
				{    Serial.println(msjin);
        				write_to_server(msjout);
				}

			_delay_ms(100);
			}
*/
                      while (status_Server() == 1){// Este while es para mantener ciclado, se puede perzonalizar la condicion de salida con algun puerto digital por ejemplo
                            write_to_server(msjout);
                            _delay_ms(100);
                      }
			disconnect_to_Server();
		}
      }
  

}

/*
/////////////////////////
SECCION CLIENTE TCP
////////////////////////
*/

//solamente revisa si esta conectado el servidor
int status_Server() {
	Cliente.flush();
	if (Cliente.connected()) {
		return 1;
	}
	else
	{
		return 0;
	}
}

// regresa el tamaÒo. del buffer, recibe el puntero del buffer 
int read_from_Server(char * msjin) {
	int z = 0;
	String c = "";
	if (Cliente.available() ) {
		c = Cliente.readString();
		c.toCharArray(msjin,c.length() +1);
		z = c.length();
		Serial.println("Mensaje Recibido: ");
		return z;
	}
	else
	{
		return 0;
	}
}

//regresa la cantidad de bytes esctritos recibe la cadena char a escribir
int write_to_server(char * msjout) {
	int z = 0;
	if (Cliente.connected())
	{
		Serial.println("Mensaje Enviado: ");
		Serial.println(msjout);
		Cliente.println(msjout);
		return z;
	}
	else
	{
		return 0;
	}
}

// Recibe la ip el puerto y cantidad de reconexiones
int connect_to_Server(IPAddress servidor, int puerto, int timeout){
	bool bandera = true;
	int error, i = 0;
	Serial.println("Conectando!");
	do
	{
		error = Cliente.connect(servidor, puerto);
		switch (error)
		{
		case 1:
			Serial.println("Cliente Conectado! ");
			Serial.println("Servidor ");
			Serial.println(servidor);
			Serial.println("Puerto ");
			Serial.println(puerto);
			delay(3000);
			bandera = false;
			break;
		case -1:
			Serial.println("Cliente Timed Out!");
			if (i <= timeout)
			{
				delay(3000);
				bandera = true;
				Serial.println("Reconectando!");
				Serial.println("Intento ");
				Serial.print(i);
			}
			else
			{
				bandera = false;

			}			

			break;
		case -2:
			Serial.println("Servidor Invalido!");
			delay(300);
			bandera = false;
			break;

		case -3:
			Serial.println("Cliente Bloqueado!");
			if (i <= timeout)
			{
				delay(3000);
				bandera = true;
				Serial.println("Reconectando!");
				Serial.println("Intento ");
				Serial.print(i);
		        }
			else
			{
				bandera = false;
			}
			break;
		case -4:
			Serial.println("Respuesta Invalida");
			if (i <= timeout)
			{
				delay(3000);
				bandera = true;
				Serial.println("Reconectando!");
				Serial.println("Intento ");
				Serial.print(i);
			}
			else
			{
				bandera = false;

			}
			break;
		default:
			Serial.println("Error Desconocido ");
			delay(3000);
			Cliente.flush();
			disconnect_to_Server();
			Cliente = Clientebk;
			bandera = false;
			break;
		}

	} while (bandera == true);
	return error;
}

//cierra si existe conexion
int disconnect_to_Server() {
	Serial.println(Cliente.connected());
	if (Cliente.connected()) {
		Serial.println("Desconectando....");
		Cliente.stop();
		return 1;
	}
	else
	{
		return -2;
	}
}

/*
/////////////////////////
////////////////////////
////////////////////////
*/
/*
/////////////////////////
SECCION SERVIDOR TELNET
////////////////////////
*/

int Convertidor() {
	bool bandera = true;
	String tempString = "";
	int i,j = 0;
	int v1, v2, v3, v4 = 0;
	int val[4] = { 0 };
	j = 0;
	for (i = 0; i < 4; i++)
	{
			tempString = tempString + textBuff[j];
			tempString = tempString + textBuff[j + 1];
			tempString = tempString + textBuff[j + 2];
			//Serial.println(tempString);
			val[i] = tempString.toInt();
			tempString = "";
			j = j + 3;
	}

	if (destino != 'm' && (val[0] > 0 && val[0] <= 254) && (val[1] >= 0 && val[1] <= 254) && (val[2] >= 0 && val[2] <= 254) && (val[3] > 0 && val[3] <= 254))
	{
		bandera = true;
	}
	else
	{
		if (destino == 'm' && (val[0] > 0 && val[0] <= 255) && (val[1] >= 0 && val[1] <= 255) && (val[2] >= 0 && val[2] <= 255) && (val[3] >= 0 && val[3] <= 254))
		{
			bandera = true;
		}
		else
		{
			bandera = false;
		}
	}
	switch (destino)
	{
	case 'i': //Establece direccion IP
		ipl[0] = val[0];
		ipl[1] = val[1];
		ipl[2] = val[2];
		ipl[3] = val[3];
		Serial.println("\nNueva IP: ");
		Serial.print(val[0]); Serial.print(".");
		Serial.print(val[1]); Serial.print(".");
		Serial.print(val[2]); Serial.print(".");
		Serial.print(val[3]); Serial.print("\n");
		ClienteT.println("\nNueva IP: ");
		ClienteT.print(val[0]); ClienteT.print(".");
		ClienteT.print(val[1]); ClienteT.print(".");
		ClienteT.print(val[2]); ClienteT.print(".");
		ClienteT.print(val[3]); ClienteT.print("\n");
		break;
	case 'd':
		dnsl[0] = val[0];
		dnsl[1] = val[1];
		dnsl[2] = val[2];
		dnsl[3] = val[3];
		Serial.println("\nNuevo DNS de Red: ");
		Serial.print(val[0]); Serial.print(".");
		Serial.print(val[1]); Serial.print(".");
		Serial.print(val[2]); Serial.print(".");
		Serial.print(val[3]); Serial.print("\n");
		ClienteT.println("\nNuevo DNS de Red: ");
		ClienteT.print(val[0]); ClienteT.print(".");
		ClienteT.print(val[1]); ClienteT.print(".");
		ClienteT.print(val[2]); ClienteT.print(".");
		ClienteT.print(val[3]); ClienteT.print("\n");
		break;
	case 'g':
		gatewayl[0] = val[0];
		gatewayl[1] = val[1];
		gatewayl[2] = val[2];
		gatewayl[3] = val[3];
		Serial.println("\nNuevo GateWay de Red: ");
		Serial.print(val[0]); Serial.print(".");
		Serial.print(val[1]); Serial.print(".");
		Serial.print(val[2]); Serial.print(".");
		Serial.print(val[3]); Serial.print("\n");
		ClienteT.println("\nNuevo GateWay de Red: ");
		ClienteT.print(val[0]); ClienteT.print(".");
		ClienteT.print(val[1]); ClienteT.print(".");
		ClienteT.print(val[2]); ClienteT.print(".");
		ClienteT.print(val[3]); ClienteT.print("\n");
		break;
	case 'm':
		subnetl[0] = val[0];
		subnetl[1] = val[1];
		subnetl[2] = val[2];
		subnetl[3] = val[3];
		Serial.println("\nNueva Mascara de Red: ");
		Serial.print(val[0]); Serial.print(".");
		Serial.print(val[1]); Serial.print(".");
		Serial.print(val[2]); Serial.print(".");
		Serial.print(val[3]); Serial.print("\n");
		ClienteT.println("\nNueva Mascra de Red: ");
		ClienteT.print(val[0]); ClienteT.print(".");
		ClienteT.print(val[1]); ClienteT.print(".");
		ClienteT.print(val[2]); ClienteT.print(".");
		ClienteT.print(val[3]); ClienteT.print("\n");
		break;
        case 's': //Establece direccion IP del servidor destino
		servidor1[0] = val[0];
		servidor1[1] = val[1];
		servidor1[2] = val[2];
		servidor1[3] = val[3];
		Serial.println("\nNueva IP del Servidor Destino: ");
		Serial.print(val[0]); Serial.print(".");
		Serial.print(val[1]); Serial.print(".");
		Serial.print(val[2]); Serial.print(".");
		Serial.print(val[3]); Serial.print("\n");
		ClienteT.println("\nNueva IP del Servidor Destino: ");
		ClienteT.print(val[0]); ClienteT.print(".");
		ClienteT.print(val[1]); ClienteT.print(".");
		ClienteT.print(val[2]); ClienteT.print(".");
		ClienteT.print(val[3]); ClienteT.print("\n");
		break;
	default:
		break;
	}

	if (bandera == true)
	{
		
		Serial.println("\nExito en Captura");
		ClienteT.println("\nExito en Captura");
		return 0;
	}
	else
	{
		Serial.println("\nFalla en Captura");
		ClienteT.println("\nFalla en Captura");
		return 1;
	}
}

void capturaTelnet( char* instruccion) {
	char c;
	bool salida = true;
	ClienteT.write("\n");
	ClienteT.println(instruccion);
	do
	{
		charsReceived = 0;	
		do {
			c = ClienteT.read();
			delayMicroseconds(10000);
			if (c > 0x2F && c < 0x3A)
			{
				textBuff[charsReceived] = c;
				//Serial.print(c);
				charsReceived++;
                                // Cambiar Este Delay
				delayMicroseconds(10000);
			}
		} while (charsReceived < textBuffSize && c != 0x0d );

		if (c == 0x0d) {
			if(Convertidor() == 0)
			{
				salida = false;
			}
			else
			{
				charsReceived = 0;
				ClienteT.println(instruccion);
				salida = true;
			}
			
		}

		if (charsReceived >= textBuffSize && salida == false) {
			charsReceived = 0;
			ClienteT.println(instruccion);
			salida = true;
		}

	} while (salida == true);
	
}

void telnet()
{
	bool bandera1 = true;
	char opcion = 0x00;
	ClienteT = ServidorT.available();
	char thisChar;
	if (ClienteT) {
		do {
			alreadyConnected = true;
			Serial.println("Entro Usuario");
			ClienteT.println("Bienvenido Erick Arzola\n");
			ClienteT.println("Selecciona una Opcion:");
			ClienteT.println("1) Establecer IP Local");
			ClienteT.println("2) Establecer Mascara de Red Local");
			ClienteT.println("3) Establecer Gateway Local");
			ClienteT.println("4) Establecer DNS Local");
			ClienteT.println("5) Habilitar DHCP Local");
			ClienteT.println("6) Establecer IP Servidor Destino");
			ClienteT.println("7) Salvar Configuracion");
			ClienteT.println("8) Desconectar ");

			if (ClienteT.available() > 0) {
				do
				{
					//Cliente.flush();
					thisChar = ClienteT.read();
					delayMicroseconds(10000);
					if (thisChar > 0x30 && thisChar < 0x39)
					{
						opcion = thisChar;
						Serial.print(thisChar);
					}

				} while (thisChar != 0x0D && thisChar != 0x1B);

				switch (opcion) {
				case '1':
					destino = 'i';
					capturaTelnet("Entra la IP Local: Formato XXX.XXX.XXX.XXX incluyendo ceros");
					bandera1 = true;
					break;
				case '2':
					destino = 'm';
					capturaTelnet("Entra la Mascara de Red Local: Formato XXX.XXX.XXX.XXX incluyendo ceros");
					bandera1 = true;
					break;
				case '3':
					destino = 'g';
					capturaTelnet("Entra el GateWay de Red Local: Formato XXX.XXX.XXX.XXX incluyendo ceros");
					bandera1 = true;
					break;
				case '4':
					destino = 'd';
					capturaTelnet("Entra el DNS de Red Local: Formato XXX.XXX.XXX.XXX incluyendo ceros");
					bandera1 = true;
					break;
				case '5': break;
				case '6': 
                                        destino = 's';
					capturaTelnet("Entra la IP del Servidor Destino: Formato XXX.XXX.XXX.XXX incluyendo ceros");
					bandera1 = true;
                                        break;
				case '7': 
					salvaDatos();
					break;
				case '8': 
					
					bandera1 = false;
					break;
				default:
					break;
				}
			}
			timeOfLastActivity = millis();
		} while (bandera1 == true && checkConnectionTimeout() == 0);
	}
//	else
//	{
//		ClienteT.stop();
//	}
}


int checkConnectionTimeout()
{
	if (millis() - timeOfLastActivity > allowedConnectTime) {
		ClienteT.println();
		ClienteT.println("Timeout Desconexxion.");
		//ClienteT.stop();
		return 1;
	}
	else
	{
		return 0;
	}
}


/*
/////////////////////////
////////////////////////
////////////////////////
*/
/*
/////////////////////////
SECCION EEPROM
////////////////////////
*/

int salvaDatos()
{
	char c = 0x00;
	bool bandera = false;
	bool exito = true;
	int i = 0;
        char tempdhcp = 0x00;
	int tempip, tempsubnet, tempdns, tempgateway, tempservidor = 0;
	Serial.println("Deseas Salvar y/n: ");
	ClienteT.println("Datos Nuevos\n:");
	ClienteT.println(ipl);
	ClienteT.println("\n");
	ClienteT.println(dnsl);
	ClienteT.println("\n");
	ClienteT.println(gatewayl);
	ClienteT.println("\n");
	ClienteT.println(subnetl);
	ClienteT.println("\n");
        ClienteT.println(servidor1);
	ClienteT.println("\n");
        if (dhcpopcion == 1){
          Serial.println("DHCP ACTIVADO ");
        }else{
          Serial.println("DHCP NO ACTIVO ");
        }
	ClienteT.println("Deseas Salvar Estos Datos y / n:");

	do
	{
		c = ClienteT.read();
		delayMicroseconds(10000);
		if (c == 'y' || c == 'Y')
		{
			bandera = true;
			delayMicroseconds(10000);
		}
		else
		{
			if (c == 'n' || c == 'N')
			{
				bandera = false;
				delayMicroseconds(10000);
			}
		}
	} while (c != 0x0d);

	if (bandera == true)
	{
		ClienteT.println("GUARDANDO...\n");

                for (i = 0; i < 4; i++){
                      EEPROM.write(IPEEPROM + i, ipl[i]);
		      delay(100);
                      EEPROM.write(GATEWAYEEPROM + i, gatewayl[i]);
                      delay(100);
                      EEPROM.write(DNSSEEPROM + i, dnsl[i]);
		      delay(100);
                      EEPROM.write(NETMASKEEPROM + i, subnetl[i]);
                      delay(100);
                      EEPROM.write(IPSERVIDOREEPROM + i, servidor1[i]);
                      delay(100);
                }
                EEPROM.write(DHCPSTATUSEEPROM, dhcpopcion);
                delay(100);
                EEPROM.write(YAESCRITO, 0x01);
                delay(100);


		exito = true;
		for (i = 0; i < 4; i++)
		{
			tempip = EEPROM.read(IPEEPROM + i);
			delay(100);
			if (tempip != ipl[i]) {
				exito = false;
			}
			tempgateway = EEPROM.read(GATEWAYEEPROM + i);
			delay(100);
			if (tempgateway != gatewayl[i]) {
				exito = false;
			}
			tempdns = EEPROM.read(DNSSEEPROM + i);
			delay(100);
			if (tempdns != dnsl[i]) {
				exito = false;
			}
			tempsubnet = EEPROM.read(NETMASKEEPROM + i);
			delay(100);
			if (tempsubnet != subnetl[i]) {
				exito = false;
			}
                        tempservidor = EEPROM.read(IPSERVIDOREEPROM + i);
			delay(100);
			if (tempservidor != servidor1[i]) {
				exito = false;
			}
		}
                tempdhcp = EEPROM.read(DHCPSTATUSEEPROM);
			delay(100);
			if (tempdhcp != dhcpopcion) {
				exito = false;
			}

		if (exito == true)
		{
			Serial.println("GUARDADO EXITOSO!!!\n");
			ClienteT.println("GUARDADO EXITOSO!!!\n");
			return 0;
		}
		else
		{
			Serial.println("!!!!FALLA AL GUARDAR LA MEMORIA!!!\n");
			ClienteT.println("!!!!FALLA AL GUARDAR LA MEMORIA!!!\n");
                        banderareset = 1;
			return 1;
		}


	}
	else
	{
                Serial.println("NO SE GUARDO NADA...\n");
		ClienteT.println("NO SE GUARDO NADA...\n");
                banderareset = 0;
		return 0;
	}


}

int recuperaDatos()
{
  char estado = 0xFF;
  int i = 0;
      estado = EEPROM.read(YAESCRITO);
      delay(100);
      if(estado == 0x01){//Significa que ya hay datos
      for (i = 0; i < 4; i++)
          {
	    ipl[i] = EEPROM.read(IPEEPROM + i);
	    delay(100);
	    gatewayl[i] = EEPROM.read(GATEWAYEEPROM + i);
	    delay(100);
	    dnsl[i] = EEPROM.read(DNSSEEPROM + i);
	    delay(100);
	    subnetl[i] = EEPROM.read(NETMASKEEPROM + i);
	    delay(100);
            servidor1[i] =EEPROM.read(IPSERVIDOREEPROM + i);
            delay(100);
          }
          dhcpopcion = EEPROM.read(DHCPSTATUSEEPROM);
          delay(100);
          Serial.println("----Valores Recuperados-----");
        return 1;
      }else{///Guarda Valores Por Defecto
              for (i = 0; i < 4; i++){
                      EEPROM.write(IPEEPROM + i, ipl[i]);
		      delay(100);
                      EEPROM.write(GATEWAYEEPROM + i, gatewayl[i]);
                      delay(100);
                      EEPROM.write(DNSSEEPROM + i, dnsl[i]);
		      delay(100);
                      EEPROM.write(NETMASKEEPROM + i, subnetl[i]);
                      delay(100);
                      EEPROM.write(IPSERVIDOREEPROM + i, servidor1[i]);
                      delay(100);
                }
                EEPROM.write(DHCPSTATUSEEPROM, dhcpopcion);
                delay(100);
                EEPROM.write(YAESCRITO, 0x01);
                delay(100);       
                Serial.println("------Guardados Valores Por Defecto-----");
        return 0;
      }
      
}

void softReset(){
asm volatile ("  jmp 0");
}


void closeSockets()
{
 
  for (int i = 0; i < MAX_SOCK_NUM; i++) {
          closer(i);
  }
 
}
void closer(SOCKET s)
{
  W5100.execCmdSn(s, Sock_CLOSE);
  W5100.writeSnIR(s, 0xFF);
}


