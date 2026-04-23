#include "Translator.h"

namespace x4 {

Translator& Translator::instance() {
    static Translator t;
    return t;
}

Translator::Translator() {
    loadEnglish();
    loadSpanish();
}

void Translator::setLanguage(const QString& lang) {
    if (translations_.contains(lang)) {
        currentLang_ = lang;
    } else {
        currentLang_ = QStringLiteral("en"); // Fallback
    }
}

QStringList Translator::availableLanguages() const {
    QStringList langs = translations_.keys();
    langs.sort();
    return langs;
}

QString Translator::languageDisplayName(const QString& lang) const {
    if (lang == QStringLiteral("en")) return QStringLiteral("English");
    if (lang == QStringLiteral("es")) return QStringLiteral("Español");
    return lang;
}

QString Translator::get(const QString& key) const {
    auto dict = translations_.value(currentLang_);
    if (dict.contains(key)) {
        return dict.value(key);
    }
    // Fallback to english
    auto enDict = translations_.value(QStringLiteral("en"));
    if (enDict.contains(key)) {
        return enDict.value(key);
    }
    return key;
}

QString Translator::get(const QString& key, const QString& a1) const {
    return get(key).arg(a1);
}

QString Translator::get(const QString& key, const QString& a1, const QString& a2) const {
    return get(key).arg(a1, a2);
}

QString Translator::get(const QString& key, int a1) const {
    return get(key).arg(a1);
}

QString Translator::get(const QString& key, int a1, int a2) const {
    return get(key).arg(a1).arg(a2);
}

QString Translator::get(const QString& key, const QString& a1, int a2) const {
    return get(key).arg(a1).arg(a2);
}

void Translator::loadEnglish() {
    QHash<QString, QString> dict;
    // General
    dict["btn.cancel"] = "Cancel";
    dict["btn.ok"] = "OK";
    dict["btn.save"] = "Save";
    dict["btn.create"] = "Create";
    dict["btn.refresh"] = "Refresh";

    // Main Window Sidebar
    dict["main.sidebar.add"] = "➕  Add Instance";
    dict["main.sidebar.import"] = "📦  Import";
    dict["main.sidebar.launch"] = "▶  Launch";
    dict["main.sidebar.kill"] = "⏹  Kill";
    dict["main.sidebar.edit"] = "✎  Edit";
    dict["main.sidebar.folder"] = "📂  Folder";
    dict["main.sidebar.accounts"] = "👤  Accounts";
    dict["main.sidebar.settings"] = "⚙  Settings";

    // Main Window Status
    dict["main.status.ready"] = "Ready";
    dict["main.status.playtime"] = "Total playtime: %1h %2m";
    dict["main.status.instances"] = "%1 instance(s)";

    // Main Window Messages
    dict["main.msg.import.title"] = "Import Instance";
    dict["main.msg.import.fail"] = "Import Failed";
    dict["main.msg.import.success"] = "Import Successful";
    dict["main.msg.import.success.desc"] = "Instance '%1' imported successfully.";
    dict["main.msg.running.title"] = "Running";
    dict["main.msg.running.desc"] = "%1 is already running.";
    dict["main.msg.kill.title"] = "Kill Instance";
    dict["main.msg.kill.desc"] = "Force kill \"%1\"?\n\nUnsaved progress will be lost.";
    dict["main.msg.del.title"] = "Delete Instance";
    dict["main.msg.del.desc"] = "Are you sure you want to delete \"%1\"?\n\nThis will permanently remove all instance files including mods, saves, and configurations.";
    dict["main.msg.clone.title"] = "Clone Instance";
    dict["main.msg.clone.prompt"] = "Name for cloned instance:";
    dict["main.msg.group.title"] = "Change Group";
    dict["main.msg.group.prompt"] = "Group name:";

    // Context Menu
    dict["ctx.launch"] = "▶  Launch";
    dict["ctx.edit"] = "✎  Edit";
    dict["ctx.group"] = "🏷  Change Group";
    dict["ctx.clone"] = "⎘  Clone";
    dict["ctx.folder"] = "📂  Open Folder";
    dict["ctx.delete"] = "🗑  Delete";

    // Create Instance
    dict["create.title"] = "New Instance";
    dict["create.basic"] = "Basic Info";
    dict["create.name"] = "Name:";
    dict["create.name.ph"] = "My Instance";
    dict["create.icon"] = "Icon:";
    dict["create.version_config"] = "Version Config";
    dict["create.mc"] = "Minecraft:";
    dict["create.snapshots"] = "Show snapshots";
    dict["create.loader"] = "Mod Loader:";
    dict["create.loader_ver"] = "Loader Version:";
    dict["create.loader.none"] = "No loader needed";
    dict["create.loader.loading"] = "Loading versions...";
    dict["create.loader.found"] = "%1 version(s) found";
    dict["create.loader.notfound"] = "No compatible versions found";
    dict["create.loader.fail"] = "Failed: %1";
    dict["create.manifest.loading"] = "Loading version list...";
    dict["create.manifest.loaded"] = "Version list loaded.";
    dict["create.manifest.fail"] = "Failed to load versions: %1";
    dict["create.manifest.empty"] = "No versions available.";
    dict["create.summary"] = "Minecraft %1 · %2";

    // Settings
    dict["settings.title"] = "Settings";
    dict["settings.tab.general"] = "General";
    dict["settings.tab.java"] = "Java";
    dict["settings.tab.mc"] = "Minecraft";
    dict["settings.tab.net"] = "Network";
    
    dict["settings.gen.theme"] = "Theme:";
    dict["settings.gen.theme.dark"] = "Dark";
    dict["settings.gen.theme.light"] = "Light";
    dict["settings.gen.theme.zen"] = "Zen";
    dict["settings.gen.zen"] = "Zen Mode (Restart required)";
    dict["settings.gen.lang"] = "Language:";
    dict["settings.gen.close"] = "On Game Launch:";
    dict["settings.gen.close.keep"] = "Keep launcher open";
    dict["settings.gen.close.min"] = "Minimize launcher";
    dict["settings.gen.close.close"] = "Close launcher";
    dict["settings.gen.log"] = "Log Level:";
    dict["settings.gen.log.info"] = "Info";
    dict["settings.gen.log.debug"] = "Debug";

    dict["settings.java.default"] = "Default Java:";
    dict["settings.java.default.ph"] = "(auto-detect)";
    dict["settings.java.auto"] = "Auto-detect Java for each Minecraft version";
    dict["settings.java.detected"] = "Detected Java Installations";
    dict["settings.java.scanning"] = "Scanning...";
    dict["settings.java.dl"] = "Download Java...";

    dict["settings.mc.mem"] = "Default Memory";
    dict["settings.mc.min"] = "Min:";
    dict["settings.mc.max"] = "Max:";
    dict["settings.mc.jvm"] = "Default JVM Args:";
    dict["settings.mc.jvm.ph"] = "-XX:+UseG1GC";
    dict["settings.mc.game"] = "Default Game Args:";
    dict["settings.mc.game.ph"] = "(e.g. --server mc.example.com)";
    dict["settings.mc.win"] = "Game Window Size";
    dict["settings.mc.width"] = "Width:";
    dict["settings.mc.height"] = "Height:";
    dict["settings.mc.fs"] = "Fullscreen";
    dict["settings.mc.snap"] = "Show snapshot versions";
    dict["settings.mc.alpha"] = "Show old alpha/beta versions";

    dict["settings.net.parallel"] = "Parallel Downloads:";

    // Crash Report
    dict["crash.title"] = "Game Crash Report";
    dict["crash.lbl.error"] = "Error Category:";
    dict["crash.lbl.cause"] = "Probable Cause:";
    dict["crash.lbl.actions"] = "Recommended Actions:";
    dict["crash.lbl.details"] = "Technical Details:";
    dict["crash.btn.copy"] = "Copy Error";
    dict["crash.btn.log"] = "Open Log File";
    dict["crash.btn.close"] = "Close";

    // Error Categories
    dict["err.cat.launch"] = "Launch Error";
    dict["err.cat.net"] = "Network Error";
    dict["err.cat.java"] = "Java Error";
    dict["err.cat.fs"] = "File System Error";
    dict["err.cat.mod"] = "Mod Loader Error";
    dict["err.cat.auth"] = "Authentication Error";
    dict["err.cat.unknown"] = "Unknown Error";

    translations_.insert("en", dict);
}

void Translator::loadSpanish() {
    QHash<QString, QString> dict;
    // General
    dict["btn.cancel"] = "Cancelar";
    dict["btn.ok"] = "Aceptar";
    dict["btn.save"] = "Guardar";
    dict["btn.create"] = "Crear";
    dict["btn.refresh"] = "Actualizar";

    // Main Window Sidebar
    dict["main.sidebar.add"] = "➕  Añadir Instancia";
    dict["main.sidebar.import"] = "📦  Importar";
    dict["main.sidebar.launch"] = "▶  Jugar";
    dict["main.sidebar.kill"] = "⏹  Forzar Cierre";
    dict["main.sidebar.edit"] = "✎  Editar";
    dict["main.sidebar.folder"] = "📂  Carpeta";
    dict["main.sidebar.accounts"] = "👤  Cuentas";
    dict["main.sidebar.settings"] = "⚙  Ajustes";

    // Main Window Status
    dict["main.status.ready"] = "Listo";
    dict["main.status.playtime"] = "Tiempo de juego: %1h %2m";
    dict["main.status.instances"] = "%1 instancia(s)";

    // Main Window Messages
    dict["main.msg.import.title"] = "Importar Instancia";
    dict["main.msg.import.fail"] = "Fallo al importar";
    dict["main.msg.import.success"] = "Importación Exitosa";
    dict["main.msg.import.success.desc"] = "La instancia '%1' se importó correctamente.";
    dict["main.msg.running.title"] = "En ejecución";
    dict["main.msg.running.desc"] = "%1 ya se está ejecutando.";
    dict["main.msg.kill.title"] = "Forzar Cierre";
    dict["main.msg.kill.desc"] = "¿Forzar el cierre de \"%1\"?\n\nSe perderá el progreso no guardado.";
    dict["main.msg.del.title"] = "Eliminar Instancia";
    dict["main.msg.del.desc"] = "¿Estás seguro de que deseas eliminar \"%1\"?\n\nEsto eliminará permanentemente todos los archivos de la instancia, incluyendo mods, mundos y configuraciones.";
    dict["main.msg.clone.title"] = "Clonar Instancia";
    dict["main.msg.clone.prompt"] = "Nombre de la instancia clonada:";
    dict["main.msg.group.title"] = "Cambiar Grupo";
    dict["main.msg.group.prompt"] = "Nombre del grupo:";

    // Context Menu
    dict["ctx.launch"] = "▶  Jugar";
    dict["ctx.edit"] = "✎  Editar";
    dict["ctx.group"] = "🏷  Cambiar Grupo";
    dict["ctx.clone"] = "⎘  Clonar";
    dict["ctx.folder"] = "📂  Abrir Carpeta";
    dict["ctx.delete"] = "🗑  Eliminar";

    // Create Instance
    dict["create.title"] = "Nueva Instancia";
    dict["create.basic"] = "Información Básica";
    dict["create.name"] = "Nombre:";
    dict["create.name.ph"] = "Mi Instancia";
    dict["create.icon"] = "Icono:";
    dict["create.version_config"] = "Configuración de Versión";
    dict["create.mc"] = "Minecraft:";
    dict["create.snapshots"] = "Mostrar snapshots";
    dict["create.loader"] = "Cargador de Mods:";
    dict["create.loader_ver"] = "Versión del Cargador:";
    dict["create.loader.none"] = "Sin cargador";
    dict["create.loader.loading"] = "Cargando versiones...";
    dict["create.loader.found"] = "%1 versión(es) encontrada(s)";
    dict["create.loader.notfound"] = "No se encontraron versiones compatibles";
    dict["create.loader.fail"] = "Error: %1";
    dict["create.manifest.loading"] = "Cargando lista de versiones...";
    dict["create.manifest.loaded"] = "Lista de versiones cargada.";
    dict["create.manifest.fail"] = "Fallo al cargar versiones: %1";
    dict["create.manifest.empty"] = "No hay versiones disponibles.";
    dict["create.summary"] = "Minecraft %1 · %2";

    // Settings
    dict["settings.title"] = "Ajustes";
    dict["settings.tab.general"] = "General";
    dict["settings.tab.java"] = "Java";
    dict["settings.tab.mc"] = "Minecraft";
    dict["settings.tab.net"] = "Red";
    
    dict["settings.gen.theme"] = "Tema:";
    dict["settings.gen.theme.dark"] = "Oscuro";
    dict["settings.gen.theme.light"] = "Claro";
    dict["settings.gen.theme.zen"] = "Zen";
    dict["settings.gen.zen"] = "Modo Zen (Requiere reiniciar)";
    dict["settings.gen.lang"] = "Idioma / Language:";
    dict["settings.gen.close"] = "Al iniciar el juego:";
    dict["settings.gen.close.keep"] = "Mantener abierto el lanzador";
    dict["settings.gen.close.min"] = "Minimizar el lanzador";
    dict["settings.gen.close.close"] = "Cerrar el lanzador";
    dict["settings.gen.log"] = "Nivel de Log:";
    dict["settings.gen.log.info"] = "Info";
    dict["settings.gen.log.debug"] = "Depuración";

    dict["settings.java.default"] = "Java por defecto:";
    dict["settings.java.default.ph"] = "(autodetectar)";
    dict["settings.java.auto"] = "Autodetectar Java para cada versión de Minecraft";
    dict["settings.java.detected"] = "Instalaciones de Java detectadas";
    dict["settings.java.scanning"] = "Escaneando...";
    dict["settings.java.dl"] = "Descargar Java...";

    dict["settings.mc.mem"] = "Memoria por defecto";
    dict["settings.mc.min"] = "Mín:";
    dict["settings.mc.max"] = "Máx:";
    dict["settings.mc.jvm"] = "Argumentos JVM:";
    dict["settings.mc.jvm.ph"] = "-XX:+UseG1GC";
    dict["settings.mc.game"] = "Argumentos del Juego:";
    dict["settings.mc.game.ph"] = "(ej. --server mc.example.com)";
    dict["settings.mc.win"] = "Tamaño de la Ventana";
    dict["settings.mc.width"] = "Ancho:";
    dict["settings.mc.height"] = "Alto:";
    dict["settings.mc.fs"] = "Pantalla Completa";
    dict["settings.mc.snap"] = "Mostrar versiones snapshot";
    dict["settings.mc.alpha"] = "Mostrar versiones alpha/beta antiguas";

    dict["settings.net.parallel"] = "Descargas Paralelas:";

    // Crash Report
    dict["crash.title"] = "Reporte de Cierre Inesperado";
    dict["crash.lbl.error"] = "Categoría de Error:";
    dict["crash.lbl.cause"] = "Causa Probable:";
    dict["crash.lbl.actions"] = "Acciones Recomendadas:";
    dict["crash.lbl.details"] = "Detalles Técnicos:";
    dict["crash.btn.copy"] = "Copiar Error";
    dict["crash.btn.log"] = "Abrir Archivo de Log";
    dict["crash.btn.close"] = "Cerrar";

    // Error Categories
    dict["err.cat.launch"] = "Error de Lanzamiento";
    dict["err.cat.net"] = "Error de Red";
    dict["err.cat.java"] = "Error de Java";
    dict["err.cat.fs"] = "Error de Sistema de Archivos";
    dict["err.cat.mod"] = "Error de Cargador de Mods";
    dict["err.cat.auth"] = "Error de Autenticación";
    dict["err.cat.unknown"] = "Error Desconocido";

    translations_.insert("es", dict);
}

} // namespace x4
