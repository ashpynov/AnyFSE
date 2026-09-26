# AnyFSE — modo Xbox com launcher personalizado

O AnyFSE permite usar Playnite, Steam Big Picture e outros launchers como aplicativo inicial da experiência de jogos em tela cheia do Windows (modo Xbox).

Esta versão modificada acrescenta otimização reversível do Windows, uma adaptação compatível do OpenGameBoost, recuperação persistente de configurações e correções na abertura dos launchers. As alterações são locais a este projeto; não correspondem necessariamente às versões publicadas pelo autor original.

- [Projeto original](https://github.com/ashpynov/AnyFSE)
- [Versões do projeto original](https://github.com/ashpynov/AnyFSE/releases/latest)
- [Comunidade do projeto original](https://discord.gg/hnVwuTzDmk)
- [Licença MIT](LICENSE)

## Requisitos

- Windows com as APIs de Gaming Full Screen Experience disponíveis. O AnyFSE verifica essa disponibilidade ao iniciar.
- Um launcher instalado separadamente.
- AnyFSE registrado como aplicativo inicial em **Configurações do Windows → Jogos → Experiência em tela cheia**, quando essa opção estiver disponível.
- Para otimizar serviços e configurações do sistema, tarefa administrativa `AnyFSE` registrada para o usuário da sessão.

O AnyFSE não implementa as APIs ausentes do modo Xbox. A opção que identifica o dispositivo como portátil de jogos permite a seleção de launcher em cenários suportados, mas não substitui os requisitos do Windows.

## Configuração recomendada

1. Abra as configurações do AnyFSE.
2. Escolha o launcher, por exemplo **Playnite Fullscreen**, e confira o caminho do executável.
3. O launcher abre sem elevação solicitada pelo AnyFSE. A opção **Start Launcher as Administrator** foi removida; configurações antigas de `as_admin` são ignoradas.
4. Para o Playnite, o argumento `--hidesplashscreen` evita sobrepor a tela de abertura nativa à do AnyFSE.
5. Entre no modo Xbox pela opção de entrada imediata, sem reinicialização.

Executar o launcher como administrador não é um requisito para otimizar o Windows. O launcher e a otimização têm fluxos separados: as alterações administrativas são executadas pela tarefa agendada. A elevação do launcher, sozinha, não aumenta o desempenho.

### “Restaurar o modo Xbox para PC”

Esse link restaura o valor original de identificação do dispositivo (`DeviceForm`) salvo pelo AnyFSE. Ele desfaz a alteração usada para permitir o aplicativo inicial personalizado. A seleção de launcher pode deixar de estar disponível após essa restauração.

Não é o comando de saída para o desktop nem o mecanismo de restauração dos serviços. Para preservar o Playnite como aplicativo inicial, mantenha a identificação necessária à seleção personalizada.

## O que acontece ao entrar e sair do modo Xbox

1. O Windows abre o AnyFSE como aplicativo inicial, ou o usuário inicia o AnyFSE quando o modo Xbox já está ativo.
2. O AnyFSE abre o launcher configurado e apresenta a tela de carregamento.
3. Um processo separado, `/OptimizationMonitor`, consulta o modo real do Windows aproximadamente a cada segundo.
4. Quando o modo Xbox está ativo, o monitor solicita as otimizações à tarefa administrativa.
5. A tela de carregamento fecha quando a janela do launcher é detectada. O monitor continua ativo independentemente dessa janela.
6. Ao voltar ao desktop, o monitor solicita a restauração dos estados anteriores.

Não há solicitação de reboot pela otimização. As opções antigas de entrar no modo Xbox com reinicialização continuam existindo e têm finalidade separada. O monitor precisa estar em execução; este mecanismo não é um serviço de inicialização automática do Windows.

## Otimizações implementadas

O perfil é seletivo e reversível. Não desliga indiscriminadamente todos os serviços e não promete aumento de FPS.

| Componente | Durante o modo Xbox | Ao voltar ao desktop |
| --- | --- | --- |
| Windows Search (`WSearch`) | Tenta interromper a indexação, se estiver ativa | Reinicia se estava ativo antes |
| SysMain | Tenta interromper o pré-carregamento | Reinicia se estava ativo antes |
| Telemetria (`DiagTrack`) | Tenta interromper o serviço | Reinicia se estava ativo antes |
| Mapas offline (`MapsBroker`) | Tenta interromper o serviço | Reinicia se estava ativo antes |
| Plano de energia | Seleciona Ultimate Performance existente; senão, Alto Desempenho existente; senão, mantém o atual | Restaura o plano original |
| Estado mínimo da CPU na tomada | Define 100% no plano selecionado, quando suportado | Restaura o índice AC anterior |
| Suspensão seletiva USB na tomada | Desativa no plano selecionado, quando suportado | Restaura o índice AC anterior |
| Economia de energia PCIe na tomada | Desativa no plano selecionado, quando suportado | Restaura o índice AC anterior |
| Game Mode / Game Bar | Define `AutoGameModeEnabled`, `AllowAutoGameMode` e `AppCaptureEnabled` como 1 | Restaura tipo e conteúdo originais; remove valores que não existiam |
| Perfil MMCSS Games | Define `Priority=6` e `Scheduling Category=High` na chave HKLM correta | Restaura os valores originais |
| Aceleração do mouse | Desativa na sessão com `SPI_SETMOUSE`, sem gravar a preferência permanente | Restaura os três parâmetros anteriores |

Os tipos de inicialização dos serviços não são alterados. Serviços originalmente parados permanecem parados; dependentes ativos não são interrompidos em cascata. O monitor não combate uma reinicialização por demanda feita pelo Windows ou por outro aplicativo.

Os ajustes de energia podem aumentar consumo e temperatura. Os índices de bateria (DC) não são reescritos, mas trocar o plano ativo também seleciona os valores DC daquele plano. Nenhum plano novo é criado.

MMCSS depende de o aplicativo usar esse mecanismo; não aumenta a prioridade de todos os jogos. Habilitar captura da Game Bar não significa iniciar gravação contínua. Valores de Registro podem ser consumidos apenas quando o componente ou jogo abre uma nova sessão.

### Adaptação do OpenGameBoost

A integração é nativa em C++, a partir do ZIP fornecido, sem instalar Python ou executar o projeto externo. A licença foi preservada em [OpenGameBoost-LICENSE.txt](docs/OpenGameBoost-LICENSE.txt).

A rotina original de restauração do Registro estava incompleta, e a restauração de rede aplicava valores padrão em vez de recuperar os valores anteriores. Essas rotinas não foram reaproveitadas: o AnyFSE registra o estado anterior antes de modificar cada opção.

Por escolha do perfil compatível, ficaram fora da aplicação automática:

- Limpeza de memória com `EmptyWorkingSet`, que não permite recompor o conjunto de páginas anterior.
- Suspensão do Explorer, navegadores, launchers, Discord, sincronizadores e utilitários de hardware.
- HAGS e outras alterações dependentes de reinicialização.
- Ajustes de Nagle/TCP ACK, NetBIOS e LLMNR, que alteram o comportamento da rede e não têm aplicação imediata garantida pela simples escrita no Registro.
- `GPU Priority` e `SFIO Priority`, valores descritos como não utilizados na documentação do MMCSS.
- Desativação da recuperação do Explorer, do serviço `xbgm` e o perfil agressivo de efeitos visuais.

Rede, áudio, Bluetooth, controles, drivers, segurança, Windows Update, serviços Xbox, licenciamento e anti-cheat não são alvos da lista de serviços interrompidos.

Consulte o [mapeamento completo da integração](docs/opengameboost-integration.md) e os [detalhes da otimização de serviços](docs/game-optimization.md).

## Backups e recuperação

Os registros de recuperação ficam em:

- `HKEY_LOCAL_MACHINE\SOFTWARE\AnyFSE\ServiceRestore`: serviços anteriormente ativos.
- `HKEY_LOCAL_MACHINE\SOFTWARE\AnyFSE\GameBoost`, valor `JournalV1`: snapshots de energia, Registro e mouse, identificação do usuário e fase da operação.

O backup é gravado antes da alteração. Repetir a entrada não sobrescreve os valores originais. Falhas de captura ou persistência impedem a alteração correspondente; falhas de restauração preservam os snapshots e provocam nova tentativa, normalmente após 30 segundos.

Uma nova sessão aguarda a conclusão de uma recuperação pendente do perfil GameBoost. Valores originalmente ausentes são removidos na restauração; chaves vazias eventualmente criadas podem permanecer. Chaves compartilhadas não são apagadas recursivamente.

Se o monitor for encerrado à força, a máquina desligar ou a tarefa ficar indisponível, a restauração imediata não é garantida. Reabra o AnyFSE no desktop, com o mesmo usuário, para tentar a recuperação. Não apague os registros de backup para “resolver” uma falha. Alterações manuais nos mesmos ajustes durante o modo Xbox serão substituídas pelos valores salvos ao restaurar.

O uso simultâneo em várias sessões interativas ainda não foi validado. Benchmarks e testes reais de todas as transições continuam necessários.

## Correções de inicialização e foco

Esta versão inclui:

- Retorno de sucesso/falha ao criar o processo do launcher.
- Falhas ao iniciar o launcher encerram a abertura com uma mensagem de erro.
- Mensagens de erro em português e inglês para falha de abertura ou ausência de janela detectável em 60 segundos.
- Fechamento da tela de carregamento com **Esc**. Isso não encerra o jogo nem solicita saída do modo Xbox.
- Encerramento da espera pelo launcher após cancelamento ou erro.
- Correção da detecção de janela minimizada com `IsIconic`.
- Preferência por ativar a janela existente, sem relançar o Playnite apenas para tentar obter foco.
- Tentativa de trazer a tela inicial para frente e transferência de foco ao launcher depois de fechar a tela de carregamento.
- Limpeza dos temporizadores e recursos de vídeo ao destruir a janela.
- Serialização das chamadas administrativas: uma chamada aguarda a anterior, em vez de ser descartada imediatamente.
- Limites de espera de 60 segundos para disponibilidade e 120 segundos para conclusão administrativa, sem encerrar à força uma tarefa que possa estar restaurando configurações.

As regras de foco do Windows ainda se aplicam. A compilação e os testes automatizados não comprovam, sozinhos, o comportamento visual em todas as transições.

## Tarefa administrativa e reparo da instalação

A tarefa `AnyFSE` é executada sob demanda, com privilégios elevados, na sessão interativa do usuário. Não possui gatilho de horário nem senha armazenada. Ela chama:

```text
C:\Program Files\AnyFSE\AnyFSE.exe /task
```

A tarefa é necessária para a otimização administrativa. O launcher abre normalmente sem depender dela.

Para uma instalação já registrada no Windows, depois de compilar a Release, execute em PowerShell como administrador, a partir da raiz do repositório:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\scripts\Repair-AnyFSEInstallation.ps1 -Configuration Release
```

O script:

1. Verifica os artefatos e recusa a substituição do monitor se há recuperação pendente.
2. Exige que janelas de configurações/carregamento estejam fechadas.
3. Faz backup dos binários e traduções substituídos em `build/InstalledBackup-<data-hora>`.
4. Encerra somente o monitor identificado da instalação, se não houver recuperação pendente.
5. Copia os quatro binários e as traduções pt-BR/en-US e compara seus hashes.
6. Registra a tarefa administrativa com permissão de leitura/execução para o usuário e alteração restrita a administradores/SYSTEM.
7. Registra o resultado em `build/installation-repair-result.txt` e tenta restaurar os arquivos anteriores se ocorrer falha.

Depois, abra novamente o AnyFSE para iniciar o monitor. O script também aceita `-Configuration Debug-Isolated`, mas usa Release por padrão. O launcher não possui mais a opção de elevação.

Esse reparo não instala o pacote de identidade do zero e não substitui o instalador original. Para uma primeira instalação, use um instalador compatível e registre o AnyFSE como aplicativo inicial. Arquivos compilados localmente precisam ser usados tanto pelo monitor quanto pela tarefa administrativa; misturar versões pode impedir o reconhecimento dos comandos.

## Compilar e depurar no VS Code

### Compilar pelo menu

Execute [Compilar.bat](Compilar.bat) na raiz e escolha **1 — Release**, **2 — Debug** ou **3 — Sair**. A janela permanece aberta ao terminar para mostrar o resultado. A Release sem assinatura fica em `build/Release`; a Debug fica em `build/Debug`. O script não instala os binários automaticamente.

No terminal, use `Compilar.bat Release` ou `Compilar.bat Debug` para executar sem menu nem pausa. O auxiliar [scripts/Build-AnyFSE.ps1](scripts/Build-AnyFSE.ps1) lê as tarefas do VS Code, ativa o ambiente configurado e retorna erro se a compilação falhar. São necessários os pré-requisitos abaixo; não é necessário executar como administrador.


Requisitos de desenvolvimento:

- Extensão oficial Microsoft C/C++ (`ms-vscode.cpptools`), recomendada em `.vscode/extensions.json`.
- Visual Studio Build Tools 2022 ou Visual Studio 2022 com ferramentas C++ x64.
- MSVC v143 e Windows SDK `10.0.26100.0`.
- Os componentes necessários estão listados em `.vsconfig`.

`.vscode/tasks.json` é a fonte dos comandos de compilação. O script `scripts/Enter-VSDeveloperEnvironment.cmd` usa `vswhere` para localizar o Visual Studio 2022 e ativar o ambiente x64, incluindo a edição Build Tools. Não depende do caminho fixo da edição Community.

| Tarefa | Finalidade / saída |
| --- | --- |
| `Build AnyFSE Debug` | Compilação padrão com **Ctrl+Shift+B**; `build/Debug` |
| `Build AnyFSE Debug Isolated` | Binários em `build/Debug-Isolated`, úteis quando o executável Debug está em uso |
| `Build AnyFSE Release Unsigned` | Release local sem certificado; `build/Release` |
| `Build AnyFSE Release` | Release com assinatura, exige certificado privado configurado no projeto |
| `Test Optimization Recovery` | Compila e executa testes de recuperação e concorrência administrativa |

As tarefas de testes executam primeiro suas dependências de preparação e compilação. A tarefa sem assinatura usa `SignBinaries=false`; o comportamento padrão das tarefas assinadas foi preservado. Não é criado um certificado em nome do autor original. A Release sem assinatura não é um instalador nem um AppX assinado.

As configurações de depuração usam o executável Debug da pasta do projeto. A entrada imediata usa `/FSENow`. O IntelliSense está configurado para C++17 e MSVC x64; se a versão/localização do compilador mudar, atualize `compilerPath` em `.vscode/c_cpp_properties.json`.

Há tarefas adicionais de pacote e instalador, com seus próprios requisitos de assinatura. OpenCppCoverage é opcional e necessário apenas para cobertura. Detalhes em [Compilação no VS Code](docs/vscode-build.md).

## Funcionalidades preservadas do projeto original

- Launchers predefinidos: Playnite Fullscreen/Desktop, Steam Big Picture/Desktop, LaunchBox BigBox, One Game Launcher, RetroBat, Armoury Crate SE, Kodi e Razer Cortex.
- Executável personalizado ou outro aplicativo Gaming Home instalado.
- Aplicativos adicionais na inicialização, com opção individual de elevação.
- Configurações navegáveis por controle.
- Vídeo, texto e imagem personalizados na tela de carregamento.
- Suporte ao remapeamento de botões ASUS ROG Ally e combinações com o botão Mode.
- Interface em inglês, francês, português brasileiro, russo e turco.

### Vídeos de abertura

Adicione arquivos MP4 ou WebM à pasta `C:\ProgramData\AnyFSE\splash`, ou selecione outro caminho nas configurações. O aplicativo escolhe vídeos dessa pasta para a abertura.

Nomes como `splash.m4000.mp4` ou `outro.5000.webm` permitem indicar a posição, em milissegundos, para repetição. O prefixo `m`/`M` dessa indicação silencia o vídeo durante o loop.

### ASUS ROG Ally e ACSE Filter

O componente ACSE Filter evita que o ASUS Optimization trate simultaneamente os botões remapeados. Ele usa um serviço e injeta `AnyFSE.ACSEFilterHook.dll` no processo `AsusOptimization.exe`, filtrando relatórios HID específicos dos botões ASUS.

Esse componente é separado do monitor de otimização e não é necessário para abrir o Playnite. O remapeamento e a injeção de DLL podem chamar a atenção de antivírus; analise uma detecção em vez de presumir que todo alerta seja falso ou desativar a proteção do Windows.

## Solução de problemas

| Sintoma | Verificação |
| --- | --- |
| Otimização administrativa não executa | Verifique/repare a tarefa `AnyFSE`; o launcher não depende dela para abrir |
| Otimização não é aplicada | Confira se a tarefa existe, está habilitada e aponta para a mesma versão do monitor |
| Tela de carregamento sem launcher | Verifique caminho, argumentos e configuração de detecção da janela; aguarde a mensagem de prazo excedido ou pressione Esc |
| Erro LNK1168 ao compilar | O executável de saída pode estar em uso; utilize a tarefa Debug Isolated ou feche a instância após restaurar os ajustes |
| Falha procurando certificado na Release | Use `Build AnyFSE Release Unsigned` para compilação local ou configure seu certificado para a distribuição assinada |
| Restauração pendente | Reabra o AnyFSE no desktop com o usuário original e a tarefa disponível; preserve os snapshots |

Quando o log está habilitado nas configurações, os arquivos ficam normalmente em `%LOCALAPPDATA%\Packages\ArtemShpynov.AnyFSE_by4wjhxmygwn4\LocalCache\logs`. Consulte as entradas `Elevated`, `Launchers`, `GameBoost` e `GameOptimization`. Log desativado não produz evidência detalhada de execução.

## Validação desta modificação

Na validação local de 26/09/2026:

- Release sem assinatura compilada com zero erros e quatro avisos em código preexistente.
- Oito cenários simulados de recuperação aprovados, incluindo falhas de captura, aplicação, persistência, restauração, reentrada e interrupções.
- Testes de concorrência administrativa aprovados: exclusividade, espera, limite de tempo e liberação da reserva.
- Release instalada com comparação de hashes dos quatro binários.
- Configuração local do launcher ajustada para `as_admin: false`.
- Tarefa administrativa registrada; o executável instalado confirmou um comando no desktop e o Agendador retornou resultado `0`.
- Monitor da versão instalada reiniciado.

O teste administrativo no desktop valida comunicação e execução, mas não exercita a aplicação completa do perfil no modo Xbox. Ainda faltam testes reais de entrada/saída, foco do Playnite, restauração de todos os ajustes, diferentes dispositivos e benchmarks. Não há medição comprovando ganho de FPS.

## Desinstalação

Volte ao desktop, permita a restauração e feche o monitor antes de remover os componentes. Use o desinstalador da instalação. Não remova os backups de recuperação enquanto houver ajustes pendentes.

Em instalações quebradas, os componentes a verificar são o pacote de identidade AnyFSE, a tarefa agendada `AnyFSE`, o serviço `ACSEFilterInjector` quando instalado e os arquivos em `Program Files\AnyFSE`. A remoção manual exige privilégios administrativos e deve considerar separadamente as configurações do usuário e os snapshots de restauração.

## Créditos e licenças

O AnyFSE original é de Artem Shpynov e colaboradores, sob [licença MIT](LICENSE).

- A integração como aplicativo inicial foi inspirada em [FullScreenExperienceShell](https://github.com/driver1998/FullScreenExperienceShell), de driver1998, apresentado à comunidade pelo usuário silicon.
- O tratamento dos botões ASUS foi inspirado em [Handheld Companion](https://github.com/Valkirie/HandheldCompanion) e [G-Helper](https://github.com/seerge/g-helper).
- Marecki e TwoTracks contribuíram com o projeto e os testes de suporte ao Xbox Ally e mapeamento de botões Steam no projeto original.
- As adaptações do OpenGameBoost preservam seu aviso de copyright de 2025 e sua [licença MIT](docs/OpenGameBoost-LICENSE.txt).

