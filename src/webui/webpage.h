#pragma once

static const char WEBPAGE_FORM[] = R"html(<!DOCTYPE html>
<html lang="ca">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Smart Garden &#x1F331;</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font:15px/1.5 sans-serif;background:#f5f9f5;display:flex;align-items:center;justify-content:center;min-height:100vh;padding:1rem}
.card{background:#fff;border-radius:12px;padding:2rem;width:100%;max-width:380px;box-shadow:0 2px 16px rgba(0,0,0,.1)}
h1{color:#2e7d32;font-size:1.2rem;margin-bottom:1.5rem}
h2{font-size:.75rem;text-transform:uppercase;letter-spacing:.08em;color:#888;margin:1.25rem 0 .5rem;padding-top:1rem;border-top:1px solid #eee}
h2:first-of-type{border-top:none;padding-top:0;margin-top:0}
label{display:block;font-size:.85rem;color:#555;margin-bottom:.3rem}
input{width:100%;padding:.6rem .8rem;border:1.5px solid #ddd;border-radius:6px;font-size:.95rem}
input:focus{border-color:#2e7d32;outline:none;box-shadow:0 0 0 3px rgba(46,125,50,.1)}
.f{margin-bottom:.75rem}
button{width:100%;margin-top:1.25rem;padding:.75rem;background:#2e7d32;color:#fff;border:none;border-radius:8px;font-size:1rem;cursor:pointer}
button:hover{background:#1b5e20}
</style>
</head>
<body>
<div class="card">
<h1>&#x1F331; Smart Garden</h1>
<form method="POST" action="/save">
<h2>WiFi</h2>
<div class="f"><label>SSID</label><input name="ssid" value="%SSID%" required></div>
<div class="f"><label>Contrasenya</label><input type="password" name="pass" placeholder="deixa buit per no canviar"></div>
<h2>MQTT Backend</h2>
<div class="f"><label>Broker (IP o hostname)</label><input name="broker" value="%BROKER%" required></div>
<div class="f"><label>Port</label><input type="number" name="port" value="%PORT%" min="1" max="65535" required></div>
<button type="submit">Desar i reiniciar</button>
</form>
</div>
</body>
</html>)html";

static const char WEBPAGE_SAVED[] = R"html(<!DOCTYPE html>
<html lang="ca">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Smart Garden &#x1F331;</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font:15px/1.5 sans-serif;background:#f5f9f5;display:flex;align-items:center;justify-content:center;min-height:100vh;padding:1rem}
.card{background:#fff;border-radius:12px;padding:2rem;width:100%;max-width:380px;box-shadow:0 2px 16px rgba(0,0,0,.1);text-align:center}
h1{color:#2e7d32;font-size:1.2rem;margin-bottom:1rem}
.icon{font-size:3rem;margin-bottom:.75rem}
p{color:#666;font-size:.95rem}
</style>
</head>
<body>
<div class="card">
<div class="icon">&#x2705;</div>
<h1>Configurat!</h1>
<p>El dispositiu reiniciarà en uns instants i es connectarà a la nova xarxa.</p>
</div>
</body>
</html>)html";
