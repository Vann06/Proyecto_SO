# guia paso a paso para conectar por ip

## antes de empezar

1. consigue un switch con energia y cables ethernet.
2. conecta todas las compus al switch.
3. decide cual compu sera el servidor.
4. el servidor debe usar la ip local de esa compu, no `127.0.0.1`.
5. usa esta red de ejemplo en toda la guia:
- servidor: `192.168.50.10`
- cliente 1: `192.168.50.11`
- cliente 2: `192.168.50.12`
- mascara: `255.255.255.0`
- puerto: `5000`

## caso 1: servidor en una compu linux

### 1. ver el nombre de la interfaz ethernet

1. abre terminal en la compu linux.
2. ejecuta:

```bash
ip addr
```

3. busca una interfaz ethernet, por ejemplo `eno1`, `enp0s31f6` o parecida.
4. no uses `lo` ni una interfaz que diga `wlan`.

### 2. configurar ip manual en linux desktop

1. abre `Settings`.
2. entra a `Network`.
3. en `Wired`, abre el engrane o configuracion de la red.
4. entra a `IPv4`.
5. cambia de `Automatic` a `Manual`.
6. escribe estos datos:
- address: `192.168.50.10`
- netmask: `255.255.255.0`
- gateway: vacio
7. guarda los cambios.
8. desconecta y vuelve a conectar el cable si la red no toma la ip de inmediato.

### 3. configurar ip manual en linux server

1. si usas ubuntu server sin interfaz grafica, la red se configura con netplan.
2. abre el archivo de netplan que tengas en `/etc/netplan/`.
3. deja la ip fija del servidor como `192.168.50.10`.
4. usa mascara `255.255.255.0`.
5. deja el gateway vacio si solo van a usar red local.
6. guarda el archivo.
7. aplica la configuracion.
8. verifica de nuevo con:

```bash
ip addr
```

9. confirma que aparezca `192.168.50.10`.

### 4. levantar el servidor

1. entra a la carpeta del proyecto.
2. compila si hace falta.
3. levanta el servidor con:

```bash
./server 5000
```

4. deja esa terminal abierta.
5. confirma que el servidor muestre que escucha en el puerto `5000`.

### 5. configurar ip manual en cada cliente

1. en cada cliente, abre la configuracion de red.
2. cambia la red ethernet a manual.
3. asigna una ip distinta en cada equipo:
- cliente 1: `192.168.50.11`
- cliente 2: `192.168.50.12`
4. usa mascara `255.255.255.0`.
5. deja el gateway vacio.
6. guarda cada cambio.

### 6. probar conectividad

1. en cada cliente abre terminal.
2. ejecuta:

```bash
ping 192.168.50.10
```

3. si responde, la red esta bien.
4. si no responde, revisa cable, ip, mascara y switch.

### 7. conectar clientes al servidor

1. en cada cliente entra a la carpeta del proyecto.
2. conecta al servidor con la ip del host:

```bash
./client nombre_usuario 192.168.50.10 5000
```

3. ejemplo para cliente 1:

```bash
./client diego 192.168.50.10 5000
```

4. ejemplo para cliente 2:

```bash
./client vianka 192.168.50.10 5000
```

## caso 2: servidor en windows usando wsl

### 1. configurar ip manual en windows

1. en la compu windows del servidor abre `Settings`.
2. entra a `Network & Internet`.
3. entra a `Ethernet`.
4. abre la conexion ethernet que esta conectada al switch.
5. busca `IP assignment`.
6. da clic en `Edit`.
7. elige `Manual`.
8. activa `IPv4`.
9. escribe estos datos:
- IP address: `192.168.50.10`
- Subnet mask: `255.255.255.0`
- Gateway: vacio
10. guarda los cambios.

### 2. activar mirrored mode en wsl

1. en windows abre el explorador de archivos.
2. entra a tu usuario.
3. crea o edita este archivo:

```text
C:\Users\TU_USUARIO\.wslconfig
```

4. escribe esto dentro:

```ini
[wsl2]
networkingMode=mirrored
```

5. guarda el archivo.
6. abre `PowerShell` o `CMD`.
7. ejecuta:

```bash
wsl --shutdown
```

8. vuelve a abrir wsl.

### 3. verificar ip dentro de wsl

1. abre ubuntu dentro de wsl.
2. ejecuta:

```bash
ip addr
```

3. revisa que la red esté activa.
4. si no ves la conexion esperada, revisa que mirrored mode quede activo.

### 4. levantar el servidor dentro de wsl

1. entra a la carpeta del proyecto.
2. compila si hace falta.
3. levanta el servidor:

```bash
./server 5000
```

4. deja esa terminal abierta.
5. confirma que el servidor escuche en `0.0.0.0` o `INADDR_ANY`.

### 5. revisar firewall de windows si no conecta

1. si el `ping` responde pero el cliente no entra, revisa el firewall de windows.
2. permite trafico entrante para el puerto `5000`.
3. si sigue fallando, prueba temporalmente desactivar el bloqueo solo para validar.

### 6. configurar ip manual en cada cliente

1. en cada cliente abre la configuracion de red.
2. cambia la red ethernet a manual.
3. asigna una ip distinta en cada equipo:
- cliente 1: `192.168.50.11`
- cliente 2: `192.168.50.12`
4. usa mascara `255.255.255.0`.
5. deja el gateway vacio.
6. guarda los cambios.

### 7. probar conectividad

1. en cada cliente abre terminal.
2. ejecuta:

```bash
ping 192.168.50.10
```

3. si responde, la red esta bien.
4. si no responde, revisa ip, firewall, mirrored mode y switch.

### 8. conectar clientes

1. en cada cliente entra a la carpeta del proyecto.
2. conecta al servidor usando la ip del host windows:

```bash
./client nombre_usuario 192.168.50.10 5000
```

## que probar antes de la presentacion

1. todos conectados al switch.
2. servidor con ip manual.
3. clientes con ip manual.
4. `ping` al servidor desde cada cliente.
5. servidor levantado.
6. clientes conectando a la ip del servidor.
7. probar `broadcast`, `dm`, `list`, `info`, `status` y `exit`.

## resumen corto

- si el servidor corre en linux, configura `192.168.50.10` en esa compu y usa `./server 5000`.
- si el servidor corre en windows con wsl, configura la ip en windows, activa `networkingMode=mirrored`, reinicia wsl y levanta `./server 5000`.
- en los clientes siempre usa la ip del servidor, nunca `127.0.0.1`.
