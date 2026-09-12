# SPDX-License-Identifier: GPL-3.0-or-later
"""Read ARM ABI requirements without installing a compiler on the owner's PC."""
from elftools.elf.elffile import ELFFile

def symbols(path,undefined):
    with open(path,'rb') as source:
        elf=ELFFile(source)
        if elf.elfclass!=32 or not elf.little_endian or elf['e_machine']!='EM_ARM':
            raise ValueError('Expected an ELF32 little-endian ARM component')
        table=elf.get_section_by_name('.dynsym')
        if table is None:raise ValueError('Dynamic symbol table missing')
        versions={}
        needed=elf.get_section_by_name('.gnu.version_r')
        if needed:
            for _,auxiliaries in needed.iter_versions():
                for item in auxiliaries:versions[item['vna_other']&0x7fff]=item.name
        defined=elf.get_section_by_name('.gnu.version_d')
        if defined:
            for definition,auxiliaries in defined.iter_versions():
                versions[definition['vd_ndx']&0x7fff]=next(auxiliaries).name
        indexes=elf.get_section_by_name('.gnu.version')
        result=set()
        for index,item in enumerate(table.iter_symbols()):
            bind=item['st_info']['bind'];missing=item['st_shndx']=='SHN_UNDEF'
            if not item.name or (undefined and (not missing or bind=='STB_WEAK')):continue
            if not undefined and (missing or bind not in ('STB_GLOBAL','STB_WEAK')):continue
            name=item.name
            version=indexes.get_symbol(index)['ndx'] if indexes else None
            if isinstance(version,int) and version&0x7fff>1:
                number=version&0x7fff
                if number not in versions:raise ValueError('Unresolved ELF symbol version')
                if name!=versions[number]:name+='@'+versions[number]
            result.add(name)
            if not undefined:result.add(item.name)
        return result

def needed_libraries(path):
    with open(path,'rb') as source:
        elf=ELFFile(source);section=elf.get_section_by_name('.dynamic')
        if section is None:raise ValueError('Dynamic section missing')
        return [tag.needed for tag in section.iter_tags() if tag.entry.d_tag=='DT_NEEDED']
