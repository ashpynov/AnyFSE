# Otimizacao reversivel no modo Xbox

O perfil agora inclui tambem a [integracao reversivel do OpenGameBoost](opengameboost-integration.md).
Este documento descreve a parte de servicos; o documento vinculado detalha energia,
Game Mode, MMCSS, mouse, recuperacao e os recursos excluidos do perfil compativel.

Ao iniciar o AnyFSE, um processo `/OptimizationMonitor` acompanha o estado real
do modo Xbox a cada segundo, inclusive quando a mudanca ocorre pela interface
do Windows. Ele permanece ativo depois que o launcher ou a janela inicial fecha.
O monitor existe por sessao de usuario e termina com o logoff.

O perfil tenta interromper `WSearch` (indexacao), `SysMain` (pre-carregamento),
`DiagTrack` (telemetria) e `MapsBroker` (mapas offline), somente se estiverem
em execucao e aceitarem parada. A lista fica em `src/App/Constants.hpp`.
Pesquisa/indexacao e mapas offline podem ficar limitados durante a sessao.
Nao ha promessa de ganho de FPS; compare tempos de quadro e carregamento no
hardware de destino, principalmente ao avaliar a interrupcao do SysMain.

Rede, audio, Bluetooth, controles, drivers, seguranca, Windows Update, Xbox,
Gaming Services, Store, licenciamento e anti-cheat permanecem fora do perfil.
Servicos com dependentes ativos nao sao interrompidos em cascata. O Windows
pode reiniciar um servico por demanda; o monitor nao fica combatendo essa decisao.

As operacoes administrativas usam a tarefa agendada `AnyFSE`, ja utilizada pelo
projeto. A instalacao precisa ter essa tarefa registrada e executar a versao
atualizada do AnyFSE. Rodar somente um executavel novo com a tarefa apontando
para uma instalacao antiga nao atualiza o manipulador administrativo.
Se a tarefa estiver ocupada ou indisponivel, o monitor registra a falha e tenta
novamente em 30 segundos.

Antes de pedir a parada, registra o servico em
`HKEY_LOCAL_MACHINE\SOFTWARE\AnyFSE\ServiceRestore`. Ao voltar ao desktop,
inicia os servicos registrados e remove cada registro somente depois de confirmar
que o servico esta em execucao. Falhas de restauracao preservam os registros e
geram novas tentativas. Servicos originalmente parados nao sao iniciados.
Nenhum tipo de inicializacao e alterado e a otimizacao nao solicita reinicializacao.
As opcoes existentes do AnyFSE para entrar em FSE com reboot continuam existindo;
use a opcao de entrada imediata para uma transicao sem reboot.

Se o monitor for encerrado a forca, a recuperacao acontece na proxima abertura
do AnyFSE em modo desktop. Nao existe recuperacao imediata enquanto o monitor
esta encerrado. Nao desinstale o programa durante a otimizacao: primeiro volte
ao desktop e confirme a restauracao nos logs `GameOptimization`.

## Validacao em Windows de teste

Compilar usando a tarefa `Build AnyFSE Debug` de `.vscode/tasks.json`.
Depois, validar com a tarefa administrativa apontando para o executavel atualizado:

1. Registrar os estados e tipos de inicializacao dos quatro servicos antes do teste.
2. Entrar no modo Xbox sem reboot; verificar nos logs quais paradas foram confirmadas.
3. Fechar o launcher e sair pelo Windows; confirmar a restauracao dos estados anteriores.
4. Repetir com um servico originalmente parado: ele deve continuar parado ao sair.
5. Alternar rapidamente entre Xbox e desktop; verificar o estado final e o registro de recuperacao.
6. Encerrar o monitor durante o modo Xbox, sair e reabrir o AnyFSE; verificar a recuperacao.
7. Indisponibilizar a tarefa administrativa em ambiente de teste; confirmar falhas nos logs,
   ausencia de alteracoes sem privilegios e nova tentativa apos restaurar a tarefa.
8. Comparar jogos representativos com e sem o perfil: FPS, tempos de quadro,
   carregamento, audio, rede, controles, Game Pass e anti-cheat.

Em 26/09/2026, a tarefa `Build AnyFSE Debug` compilou com sucesso depois da
instalacao dos Build Tools 2022 e Windows SDK: zero erros e quatro avisos em
codigo preexistente do componente ACSEFilterInjector.
Os testes de transicao e desempenho acima ainda precisam ser executados.
