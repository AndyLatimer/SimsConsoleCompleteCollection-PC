# Controles

Mapeo leído de `sdk/runtime/core/input.cpp`, sin prueba local de hardware.
SDL3 detecta hasta cuatro mandos y les asigna el primer puerto libre; registra conexión y
desconexión. El soporte publicado acredita un DualSense en Windows, no dos jugadores.

| DualSense | Entrada Xbox original |
| --- | --- |
| Cruz / Círculo / Cuadrado / Triángulo | A / B / X / Y |
| R1 / L1 | Negro / Blanco |
| L2 / R2 | Gatillos analógicos izquierdo / derecho |
| Options / Create | Start / Back |
| Cruceta | Cruceta |
| Pulsar L3 / R3 | Clic de stick izquierdo / derecho |
| Sticks | Sticks, eje Y convertido a la convención Xbox |

El runtime envía vibración mediante SDL; falta verificarla en hardware. No hay zona muerta
configurable, tratamiento del botón táctil ni iconografía PlayStation implementados aquí.
F1/F2 y teclado/ratón se conservan; la navegación completa de ajustes con mando está pendiente.
El juego original sí muestra teclado virtual en el recorrido upstream de Crear un Sim;
introducir nombres sin teclado y comprobar acciones reales de los botones sigue pendiente.
