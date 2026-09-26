# OpenGameBoost no AnyFSE

Perfil reversivel e compativel escolhido pelo usuario. A integracao e nativa em
C++: nao instala Python, nao executa o ZIP e nao depende do OpenGameBoost em execucao.
Origem: `OpenGameBoost-main.zip`, fornecido pelo usuario; licenca MIT preservada
em [OpenGameBoost-LICENSE.txt](OpenGameBoost-LICENSE.txt).

## Entrada e saida

O monitor existente inicia com o AnyFSE e consulta o estado real do modo Xbox a
cada segundo. Aplica o perfil quando o modo esta ativo, inclusive se o AnyFSE for
aberto depois da entrada no modo Xbox. Restaura ao voltar ao desktop, mesmo que
a janela do launcher ja tenha fechado. Nao precisa detectar um executavel de jogo.

Operacoes administrativas continuam usando a tarefa agendada `AnyFSE`. O monitor
e a tarefa precisam executar a mesma versao atualizada. A compilacao sozinha nao
atualiza uma instalacao existente nem substitui um monitor que ja esteja rodando.

## Mapeamento do projeto original

| Recurso | Integracao no perfil automatico |
| --- | --- |
| Plano de energia | Ultimate Performance existente, senao Alto Desempenho existente; mantem o atual quando esses planos nao existem. Salva e restaura o GUID original. Nao cria planos. |
| CPU minima na tomada | 100% no plano escolhido; salva/restaura o valor anterior desse plano. Pode aumentar consumo e temperatura. |
| Suspensao seletiva USB na tomada | Desativada temporariamente quando a opcao existe; restaura o valor anterior. |
| Economia de energia PCIe na tomada | Desativada temporariamente quando a opcao existe; restaura o valor anterior. |
| Game Mode/Game Bar | AutoGameModeEnabled=1, AllowAutoGameMode=1, AppCaptureEnabled=1, como no perfil normal original. Captura habilitada nao significa gravacao continua nem ganho de desempenho. |
| Perfil MMCSS Games | Priority=6 e Scheduling Category=High na chave HKLM correta. Beneficio depende de o aplicativo usar MMCSS; nao equivale a aumentar a prioridade de todos os jogos. |
| Aceleracao do mouse | Desativada na sessao por SPI_SETMOUSE. Restaura os tres parametros originais; nao muda velocidade do ponteiro nem grava preferencias permanentes do mouse. |
| Servicos da primeira integracao | Mantidos: WSearch, SysMain, DiagTrack e MapsBroker, com registro de recuperacao separado. |
| HAGS / HwSchMode | Excluido: nao e uma alternancia efetiva por sessao sem reboot. O original tambem rotula incorretamente esse mesmo ajuste como desativacao de otimizacoes de tela cheia. |
| GPU Priority e SFIO Priority | Excluidos: a documentacao do MMCSS informa que esses valores nao sao utilizados. |
| Nagle/TCP ACK, NetBIOS e LLMNR | Excluidos do perfil compativel: alteram comportamento/conectividade de rede, e uma escrita no Registro nao garante efeito imediato. Nao reinicia adaptadores nem servicos DNS durante os jogos. |
| Limpeza de memoria / EmptyWorkingSet | Excluida: nao permite recompor o conjunto de paginas anterior e pode provocar novas faltas de pagina. |
| Suspensao do Explorer, navegadores e launchers | Excluida: preserva desktop, autenticacao via navegador, downloads, overlays, DRM e a capacidade de abrir/fechar jogos. |
| Suspensao de Discord, sincronizacao e utilitarios de hardware | Excluida: preserva comunicacao, arquivos abertos e controles do hardware. |
| AutoRestartShell, desativacao xbgm/Game DVR e VisualFXSetting | Perfil agressivo excluido; nao desativa recuperacao do shell nem servicos Xbox. |

Alteracoes AC nao modificam os indices de bateria (DC), mas trocar o plano ativo
pode alterar o comportamento na bateria de acordo com os valores DC desse plano.
O perfil nao promete aumento de FPS ou reducao de latencia. Mudancas de Registro
podem ser consumidas apenas quando o jogo ou componente abre uma nova sessao.

## Restauracao e falhas

O original possui uma rotina de restauracao do Registro com `pass`, que nao restaura
os valores, e a restauracao de rede impunha padroes. Essas rotinas nao foram copiadas.

O AnyFSE registra os valores originais antes de cada alteracao, com confirmacao de
gravacao em disco. O registro fica em
`HKEY_LOCAL_MACHINE\SOFTWARE\AnyFSE\GameBoost`, valor binario `JournalV1`.
O conteudo inclui versao, usuario dono, fase e snapshots. Para valores do Registro,
preserva existencia, tipo e bytes; remove o valor ao sair se ele nao existia antes.
Chaves vazias criadas durante a sessao podem permanecer; nenhuma chave compartilhada
e apagada recursivamente. O perfil nao executa caminhos ou comandos vindos do registro.

Repetir a entrada nao sobrescreve o backup. Falha de leitura ou de gravacao do
backup impede a respectiva alteracao. Falha ao aplicar ou restaurar conserva o
snapshot para nova tentativa. A restauracao ocorre na ordem inversa; uma nova
sessao aguarda a recuperacao pendente antes de produzir novos backups. O monitor
tenta novamente em 30 segundos quando uma operacao falha.

O registro de recuperacao pertence ao usuario que aplicou o perfil: outro usuario
nao pode restaura-lo como se fosse dele. O uso simultaneo em multiplas sessoes
interativas nao foi validado. Nao altere manualmente os mesmos ajustes durante a
sessao esperando que essas mudancas sejam preservadas: a saida restaura o snapshot.

Encerrar o monitor a forca ou desligar a maquina interrompe a restauracao imediata.
Os snapshots permanecem; reabrir o AnyFSE com o mesmo usuario no desktop tenta a
recuperacao. Se um plano for apagado externamente, o snapshot fica pendente em vez
de escolher arbitrariamente outro plano. Logs: `GameBoost` e `GameOptimization`.

## Validacao

`Test Optimization Recovery` executa testes C++ com um backend em memoria, sem
alterar o Windows: backup antes da escrita, restauracao de bytes/ausencia, chamadas
repetidas, falha de captura, falha de persistencia, falha de aplicacao, interrupcao
durante gravacao, falha de restauracao, reentrada e cancelamento por mudanca de modo.

A validacao real deve comparar antes/depois: plano ativo, indices AC do plano
selecionado, valores e tipos do Registro, parametros SPI_GETMOUSE e estados dos
servicos. Repetir com falha da tarefa administrativa e encerramento do monitor.
Esses testes reais e benchmarks de jogos ainda nao foram executados.

Fontes de comportamento das APIs:
- https://learn.microsoft.com/en-us/windows/win32/procthread/multimedia-class-scheduler-service
- https://learn.microsoft.com/en-us/windows/win32/api/powersetting/nf-powersetting-powersetactivescheme
- https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-systemparametersinfow
