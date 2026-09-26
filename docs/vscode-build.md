# Compilar no VS Code (Windows)

Ambiente configurado com a extensao Microsoft C/C++ (`ms-vscode.cpptools`),
Visual Studio Build Tools 2022, MSVC v143 x64 e Windows SDK 10.0.26100.0.
A lista de componentes esta em `.vsconfig`, e a recomendacao da extensao esta
em `.vscode/extensions.json`.

1. Abra a pasta raiz do projeto no VS Code.
2. Pressione **Ctrl+Shift+B** para executar **Build AnyFSE Debug**.
3. Os arquivos gerados ficam em `build/Debug`.
4. Para depurar, escolha **Debug AnyFSE Settings** em Executar e Depurar e pressione **F5**.

As tarefas usam `scripts/Enter-VSDeveloperEnvironment.cmd` para localizar o
Visual Studio 2022 via `vswhere` e ativar o ambiente x64. Nao e necessario abrir
o VS Code pelo Developer Command Prompt. Os comandos de compilacao continuam
centralizados em `.vscode/tasks.json`.

O caminho do compilador para IntelliSense em `.vscode/c_cpp_properties.json`
corresponde a instalacao local. Se trocar a versao do MSVC, atualize esse campo
por **C/C++: Edit Configurations (UI)**. A deteccao usada pela compilacao e automatica.

As tarefas Release do projeto exigem certificado de assinatura com chave privada
configurado em `AnyFSE.Version.props`. A compilacao Debug nao exige esse certificado.
Para compilar localmente sem certificado, execute **Build AnyFSE Release Unsigned**.
Essa tarefa gera os binarios em `build/Release` com `SignBinaries=false`; nao gera
um instalador nem um pacote AppX assinado. A tarefa Release original continua exigindo assinatura.
OpenCppCoverage e opcional e so e necessario para a tarefa de cobertura.

Referencia: https://code.visualstudio.com/docs/cpp/config-msvc
