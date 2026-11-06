# Variante USA aportada — 6 de noviembre de 2026

El disco USA aportado contiene `default.xbe` SHA-1
`9e0526fc2ec6ad4896363486a404949ee3bea53f`, distinto del upstream
`40ab6f97e8ee3bf4102a5a7820dcb8047fc30097`.
No se ha modificado la ISO ni normalizado el XBE para forzar una coincidencia.

El escaneo con el SDK fijado reproduce las direcciones de **los 128 símbolos no nulos**
de `config/game.toml`, sin discrepancias. El punto de entrada `0xC7B1E`, los once rangos
de secciones, tamaño, XDK 5233, título, región USA, fechas y ruta de depuración coinciden
con los metadatos upstream en `xbe-analysis.md`.

Se ha inspeccionado también el código en las direcciones de los hooks de cuantización,
tick, zoom, menú y reconocimiento de trucos. Los accesos a `0x324220`, `0x20C14C`,
`0x21A1E8`, `0x21A294` y campos de cámara conservan el significado documentado.
La causa de la diferencia del hash completo no está determinada: no se afirma identidad
byte a byte con el XBE upstream, que no está disponible aquí.

Por esa evidencia estática se admite **este hash exacto** como variante USA para compilar
y realizar pruebas. No se admite cualquier XBE del mismo título/XDK ni se desactiva la
barrera contra PAL. CMake incorpora el hash real utilizado en el ejecutable.

Informes privados en `out/audit/usa-xbe.json`, `usa-scanned.toml`,
`usa-symbol-scan.log`, `usa-compatibility-static.json` y `usa-hook-disassembly.txt`.
La autorización estática para compilar no equivale a una prueba de jugabilidad.
