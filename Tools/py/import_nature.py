# Copyright 2026 Anderson. All Rights Reserved.
"""Importa a mata (Kenney Nature Kit, CC0) de ArtSource/Nature para /Game/Environment/Nature."""

import os
import unreal

ORIGEM = os.path.join(unreal.Paths.project_dir(), "ArtSource", "Nature")
DESTINO = "/Game/Environment/Nature"

arquivos = sorted(f for f in os.listdir(ORIGEM) if f.lower().endswith(".fbx"))
unreal.log("mata: %d arquivos em %s" % (len(arquivos), ORIGEM))

tarefas = []
for nome in arquivos:
    tarefa = unreal.AssetImportTask()
    tarefa.set_editor_property("filename", os.path.join(ORIGEM, nome))
    tarefa.set_editor_property("destination_path", DESTINO)
    tarefa.set_editor_property("automated", True)
    tarefa.set_editor_property("save", True)
    tarefa.set_editor_property("replace_existing", True)
    tarefas.append(tarefa)

unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tarefas)

registro = unreal.AssetRegistryHelpers.get_asset_registry()
registro.wait_for_completion()
importados = registro.get_assets_by_path(DESTINO, recursive=True)
malhas = [a for a in importados if a.asset_class_path.asset_name == "StaticMesh"]
unreal.log("mata: %d malhas em %s" % (len(malhas), DESTINO))
for a in sorted(malhas, key=lambda x: str(x.asset_name)):
    malha = a.get_asset()
    caixa = malha.get_bounding_box()
    unreal.log("mata: %s  caixa=%.1f x %.1f x %.1f" % (
        a.asset_name,
        caixa.max.x - caixa.min.x,
        caixa.max.y - caixa.min.y,
        caixa.max.z - caixa.min.z))
