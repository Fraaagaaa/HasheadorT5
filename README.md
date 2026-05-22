# [Descarga](https://github.com/Fraaagaaa/HasheadorT5/releases/latest/download/HasheadorT5.dll)
# HasheadorT5
Herramienta para flashear los hashes de los archivos del Call Of Duty Black Ops automáticamente en el cliente de plutonium

## Como instalar
1. Descarga el [`HasheadorT5.dll`](https://github.com/Fraaagaaa/HasheadorT5/releases/latest/download/HasheadorT5.dll).
2. Dirigete a la carpeta `%LocalAppData%/Plutonium`
3. Crea una carpeta `plugins`
4. Pon el `HasheadorT5.dll` dentro de la carpeta `plugins`
5. Inicia el juego en modo LAN


# Que hace?
A pesar de no haber sido votado, es necesario mostrar los hashes de los scripts para que validen tu partida en Zombie World Records.
Esto es una mierda porque no vale de nada y lo hay que hacer manualmente o usar herramientas externas.
<br><br>
Si no te fias de las herramientas externas no te culpo, la única que hay hasta ahora viene de alguien que ha doxeado gente y tiene un historial bastante malo en su reputación.
Este `.dll` lo puedes construir desde 0, las únicas dependencias son [minhook](https://github.com/TsudaKageyu/minhook/tree/05c06c5bbca226b72ffb40fc0caaef33bcaf6f74) y el [SDK de plutonium](https://plutonium.pw/docs/modding/plugin-sdk/)
<br><br>
La función de este mod es simple:
1. Cada 10 rondas flashea los scripts
2. A partir de la ronda 200 los flashea cada 5 rondas
3. En las rondas 3, 28, 48, 68, 98, 148 y 198 flasheará el hash de todos los archivos del juego cargados en el mapa
4. Al terminal la partida (si es por muerte, por reset no lo hará)
