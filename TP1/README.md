# Conversores AD y DA. Protocolos de comunicación serial cableados

## Trama de computadora -> Arduino (Comandos de los LEDs)

Necesitamos enviar 4 valores:

- el brillo de los tres LEDs PWM (que va de 0 a 255);
- el estado del LED 13 (0 o 1).

Hacemos un string de 10 caracteres:

- Bytes 0-2: Valor LED 9 (Ej: 000 a 255)

- Bytes 3-5: Valor LED 10 (Ej: 000 a 255)

- Bytes 6-8: Valor LED 11 (Ej: 000 a 255)

- Byte 9: Estado LED 13 (Ej: 0 o 1)

Ejemplo de trama: El string 2551270001 -> LED9: 255, LED10: 127, LED11: 000, LED13: 1

## Trama de Arduino -> Computadora (Datos del sensor LDR)

El Arduino envia los datos del sensor LDR.
analogRead() devuelve valores entre 0 y 1023 -> trama de longitud fija de 4 caracteres, rellenando con ceros a la izquierda si es necesario.

- Bytes 0-3: Valor LDR (Ej: 0250 o 1023)

## Init Servidor-Cliente

```bash
uvicorn main:app --reload
```

Luego, en el navegador, ingresar a: http://localhost:8000
