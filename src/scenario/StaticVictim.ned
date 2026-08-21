package echosar.scenario;

simple StaticVictim
{
    parameters:
        // Sem default, cada vítima precisava ser declarada uma a uma no INI, o
        // que impedia varrer o número de vítimas. Os defaults abaixo sorteiam
        // posição e instante de detecção; os cenários que precisam de
        // geometria exata continuam atribuindo explicitamente.
        string victimId = default("victim" + string(index));
        double positionX @unit(m) = default(uniform(0m, 1000m));
        double positionY @unit(m) = default(uniform(0m, 1000m));
        double positionZ @unit(m) = default(0m);
        // Teto de 700 s para que o TTL de 180 s de todo alerta caiba nos 900 s.
        double detectionTime @unit(s) = default(uniform(2s, 700s));
        @class(echosar::StaticVictim);
        @display("i=abstract/person,red;is=s");
}
