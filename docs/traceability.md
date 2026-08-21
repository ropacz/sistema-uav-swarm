# Rastreabilidade do piloto

| Elemento | Implementação | Evidência |
|---|---|---|
| Controle causal | `HypothesisPilot_BaOff/On` | única diferença: `baEnabled` |
| Obstrução física | `hypothesis-pilot-obstacle.xml` | interseções do meio e sensor |
| Trajetória da equipe | `hypothesis-pilot-team.xml` | 1,5 m/s através do edifício |
| Gatilho | `DroneApp::detectDegradation` | PDR, RSSI, silêncio e ausência de AppACK |
| Confirmação independente | `AbstractObstacleSensor` | linha de visada geométrica e alcance |
| Previsão da equipe | `DroneApp::estimateTeamPosition` | idade e quantidade das previsões |
| Otimização | `BatAlgorithm`, `RepositionFitness` | ativações, distância e viabilidade |
| Movimento | `BaGaussMarkovMobility` | deslocamento gradual, sem teletransporte |
| Entrega | `VictimAlert`/`VictimAck` | AppACK e expiração |
| Ausência de multihop | `TeamApp::hopCount` | média igual a zero nas entregas |
| Reprodução | `make hypothesis-pilot` | cinco pares e relatório automático |
