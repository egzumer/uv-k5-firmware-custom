# -*- coding: utf-8 -*-

import logging
import lxml
import os
import os.path
import re
import requests

from AdvancedHTMLParser import AdvancedHTMLParser
from glob import iglob
from urllib.parse import urlparse

from cmsis.PackLint import PackLinter, VersionParser
from cmsis.Pack import Pack, Api, SemanticVersion

def create():
  return CmsisPackLinter()

class CmsisPackVersionParser(VersionParser):
  def __init__(self, logger = None):
    super().__init__(logger)
    self._packs = {}

  def _file_version_(self, file):
    v = self._regex_(file, ".*@version\s+([vV])?([0-9]+[.][0-9]+([.][0-9]+)?).*", 2)
    if not v:
      v = self._regex_(file, ".*\$Revision:\s+([vV])?([0-9]+[.][0-9]+([.][0-9]+)?).*", 2)
    return v
  
  def _cmtable_(self, file, skip = 0):
    table = ""
    dump = False
    with open(file, 'r') as f:
      for l in f:
        if not dump and l.strip() == "<table class=\"cmtable\" summary=\"Revision History\">":
          if skip > 0:
            skip -= 1
          else:
            dump = True
        if dump:
          table += l.replace("<br>", "\\n").replace("\\<", "&lt;").replace("\\>", "&gt;")
          if l.strip() == "</table>":
            break
    if table:
      table = lxml.etree.fromstring(table)
      return table
    return None
    
  def _revhistory_(self, file, skip = 0):
    table = self._cmtable_(file, skip)
    if table is not None:
      m = re.match("[Vv]?(\d+.\d+(.\d+)?)", table[1][0].text)
      if m:
        return SemanticVersion(m.group(1))
    else:
      self._logger.info("Revision History table not found in "+file)
    return None

  def readme_md(self, file):
    """Get the latest release version from README.md"""
    return self._regex_(file, ".*repository contains the CMSIS Version ([0-9]+[.][0-9]+([.][0-9]+)?).*")

  def _dxy(self, file):
    """Get the PROJECT_NUMBER from a Doxygen configuration file."""
    return self._regex_(file, "PROJECT_NUMBER\s*=\s*\"(Version\s+)?(\d+.\d+(.\d+)?)\"", 2)
    
  def _pdsc(self, file, component = None):
    pack = None
    if not file in self._packs:
      pack = Pack(file, None)
      self._packs[file] = pack
    else:
      pack = self._packs[file]
    if component:
      history = pack.history()
      for r in sorted(history.keys(), reverse=True):
        m = re.search(re.escape(component)+"(:)?\s+[Vv]?(\d+.\d+(.\d+)?)", history[r], re.MULTILINE)
        if m:
          return SemanticVersion(m.group(2))
    else:
      return pack.version()

  def _h(self, file):
    return self._file_version_(file)

  def _c(self, file):
    return self._file_version_(file)

  def _s(self, file):
    return self._file_version_(file)
    
  def _xsd(self, file, rev=False, history=False):
    if rev:
      return self._all_(file)
    elif history:
      return self._regex_(file, ".*[0-9]+\. [A-Z][a-z]+ [12][0-9]+: (v)?(\d+.\d+(.\d+)?).*", 2)
    else:
      xsd = lxml.etree.parse(str(file)).getroot()
      return SemanticVersion(xsd.get("version", None))

  def overview_txt(self, file, skip = 0):
    return self._revhistory_(file, skip)
    
  def introduction_txt(self, file, component = None):
    table = self._cmtable_(file)
    if table is None:
      return None

    if component:
      m = re.search(re.escape(component)+"\s+[Vv]?(\d+.\d+(.\d+)?)", table[1][1].text, re.MULTILINE)
      if m:
        return SemanticVersion(m.group(1))
    else:
      return SemanticVersion(table[1][0].text)
    
  def dap_txt(self, file, skip = 0):
    return self._revhistory_(file, skip)

  def general_txt(self, file, skip = 0):
    return self._revhistory_(file, skip)
  
  def history_txt(self, file, skip = 0):
    return self._revhistory_(file, skip)

  def _all_(self, file):
    """Get the version or revision tag from an arbitrary file."""
    version = self._regex_(file, ".*@version\s+([vV])?([0-9]+[.][0-9]+([.][0-9]+)?).*", 2)
    if not version:
      version = self._regex_(file, ".*\$Revision:\s+([vV])?([0-9]+[.][0-9]+([.][0-9]+)?).*", 2)
    return version
    
class CmsisPackLinter(PackLinter):

  def __init__(self, pdsc = "ARM.CMSIS.pdsc"):
    super().__init__(pdsc)
    self._versionParser = CmsisPackVersionParser(self._logger)
    
  def pack_version(self):
    return self._pack.version()
  
  def cmsis_corem_component(self):
    rte = { 'components' : set(), 'Dcore' : "Cortex-M3", 'Dvendor' : "*", 'Dname' : "*", 'Dtz' : "*", 'Dsecure' : "*", 'Tcompiler' : "*", 'Toptions' : "*" }
    cs = self._pack.component_by_name(rte, "CMSIS.CORE")
    cvs = { SemanticVersion(c.version()) for c in cs }
    if len(cvs) > 1:
      self.warning("Not all CMSIS-Core(M) components have same version information: %s", str([ (c.name(), c.version()) for c in cs ]))
    return cvs.pop()

  def cmsis_corea_component(self):
    rte = { 'components' : set(), 'Dcore' : "Cortex-A9", 'Dvendor' : "*", 'Dname' : "*", 'Dtz' : "*", 'Dsecure' : "*", 'Tcompiler' : "*", 'Toptions' : "*" }
    cs = self._pack.component_by_name(rte, "CMSIS.CORE")
    cvs = { SemanticVersion(c.version()) for c in cs }
    if len(cvs) > 1:
      self.warning("Not all CMSIS-Core(A) components have same version information: %s", str([ (c.name(), c.version()) for c in cs ]))
    return cvs.pop()

  def cmsis_rtos2_api(self):
    cs = self._pack.components_by_name("CMSIS.RTOS2")
    cvs = { SemanticVersion(c.version()) for c in cs }
    if len(cvs) > 1:
      self.warning("Not all CMSIS-RTOS2 APIs have same version information: %s", str([ (c.name(), c.version()) for c in cs ]))
    return cvs.pop()

  def cmsis_rtx5_component(self):
    cs = self._pack.components_by_name("CMSIS.RTOS2.Keil RTX5*")
    cvs = { (SemanticVersion(c.version()), SemanticVersion(c.apiversion())) for c in cs }
    if len(cvs) == 1:
      return cvs.pop()
    elif len(cvs) > 1:
      self.warning("Not all RTX5 components have same version information: %s", str([ (c.name(), c.version(), c.apiversion()) for c in cs ]))
    return None, None

  def check_general(self):
    """CMSIS version"""
    v = self.pack_version()
    self.verify_version("README.md", v)
    self.verify_version("CMSIS/DoxyGen/General/general.dxy", v)
    self.verify_version("CMSIS/DoxyGen/General/src/introduction.txt", v)

  def check_build(self):
    """CMSIS-Build version"""
    v = self._versionParser.get_version("CMSIS/DoxyGen/Build/Build.dxy")
    self.verify_version("CMSIS/DoxyGen/Build/src/General.txt", v)
    self.verify_version("CMSIS/DoxyGen/General/src/introduction.txt", v, component="CMSIS-Build")
    self.verify_version(self._pack.location(), v, component="CMSIS-Build")

  def check_corem(self):
    """CMSIS-Core(M) version"""
    v = self.cmsis_corem_component()
    self.verify_version("CMSIS/DoxyGen/Core/core.dxy", v)
    self.verify_version("CMSIS/DoxyGen/Core/src/Overview.txt", v)
    self.verify_version("CMSIS/DoxyGen/General/src/introduction.txt", v, component="CMSIS-Core (Cortex-M)")
    self.verify_version(self._pack.location(), v, component="CMSIS-Core(M)")

  def check_corea(self):
    """CMSIS-Core(A) version"""
    v = self.cmsis_corea_component()
    self.verify_version("CMSIS/DoxyGen/Core_A/core_A.dxy", v)
    self.verify_version("CMSIS/DoxyGen/Core_A/src/Overview.txt", v)
    self.verify_version("CMSIS/DoxyGen/General/src/introduction.txt", v, component="CMSIS-Core (Cortex-A)")
    self.verify_version(self._pack.location(), v, component="CMSIS-Core(A)")

  def check_dap(self):
    """CMSIS-DAP version"""
    v = self._versionParser.get_version("CMSIS/DoxyGen/DAP/dap.dxy")
    self.verify_version("CMSIS/DoxyGen/DAP/src/dap.txt", v)
    self.verify_version("CMSIS/DoxyGen/General/src/introduction.txt", v, component="CMSIS-DAP")
    self.verify_version(self._pack.location(), v, component="CMSIS-DAP")

  def check_driver(self):
    """CMSIS-Driver version"""
    v = self._versionParser.get_version("CMSIS/DoxyGen/Driver/Driver.dxy")
    self.verify_version("CMSIS/DoxyGen/Driver/src/General.txt", v)
    self.verify_version("CMSIS/DoxyGen/General/src/introduction.txt", v, component="CMSIS-Driver")
    self.verify_version(self._pack.location(), v, component="CMSIS-Driver")

  def check_dsp(self):
    """CMSIS-DSP version"""
    v = self._versionParser.get_version("CMSIS/DoxyGen/DSP/dsp.dxy")
    self.verify_version("CMSIS/DoxyGen/DSP/src/history.txt", v)
    self.verify_version("CMSIS/DoxyGen/General/src/introduction.txt", v, component="CMSIS-DSP")
    self.verify_version(self._pack.location(), v, component="CMSIS-DSP")

  def check_nn(self):
    """CMSIS-NN version"""
    v = self._versionParser.get_version("CMSIS/DoxyGen/NN/nn.dxy")
    self.verify_version("CMSIS/DoxyGen/NN/src/history.txt", v)
    self.verify_version("CMSIS/DoxyGen/General/src/introduction.txt", v, component="CMSIS-NN")
    self.verify_version(self._pack.location(), v, component="CMSIS-NN")

  def check_pack(self):
    """CMSIS-Pack version"""
    v = self._versionParser.get_version("CMSIS/Utilities/PACK.xsd")
    self.verify_version("CMSIS/Utilities/PACK.xsd:Revision", v, rev=True)
    self.verify_version("CMSIS/Utilities/PACK.xsd:History", v, history=True)
    self.verify_version("CMSIS/DoxyGen/Pack/Pack.dxy", v)
    self.verify_version("CMSIS/DoxyGen/Pack/src/General.txt", v)
    self.verify_version("CMSIS/DoxyGen/General/src/introduction.txt", v, component="CMSIS-Pack")
    self.verify_version(self._pack.location(), v, component="CMSIS-Pack")

  def check_rtos2(self):
    """CMSIS-RTOS2 version"""
    api = self.cmsis_rtos2_api()
    v, a = self.cmsis_rtx5_component()
    self.verify_version("CMSIS/DoxyGen/RTOS2/rtos.dxy", api)
    self.verify_version("CMSIS/DoxyGen/RTOS2/src/history.txt", api, skip=0)
    self.verify_version("CMSIS/DoxyGen/General/src/introduction.txt", api, component="CMSIS-RTOS")
    # self.verify_version(self._pack.location(), v, component="CMSIS-RTOS2")
    if a and not api.match(a):
      self.warning("RTX5 API version (%s) does not match RTOS2 API version (%s)!", a, api)
    self.verify_version("CMSIS/DoxyGen/RTOS2/src/history.txt", v, skip=1)
    
  def check_files(self):
    """Files referenced by pack description"""
    # Check schema of pack description
    self.verify_schema(self._pack.location(), "CMSIS/Utilities/PACK.xsd")
        
    # Check schema of SVD files
    svdfiles = { d.svdfile() for d in self._pack.devices() if d.svdfile() }
    for svd in svdfiles:
      if os.path.exists(svd):
        self.verify_schema(svd, "CMSIS/Utilities/CMSIS-SVD.xsd")
      else:
        self.warning("SVD File does not exist: %s!", svd)

    # Check component file version
    for c in self._pack.components():
      cv = c.version()
      for f in c.files():
        hv = f.version()
        if c is Api:
          if f.isHeader():
            if not hv:
              self.verify_version(f.location(), cv)
        if hv:
          self.verify_version(f.location(), SemanticVersion(hv))
  
  def check_doc(self, pattern="./CMSIS/Documentation/**/*.html"):
    """Documentation"""
    self.debug("Using pattern '%s'", pattern)
    for html in iglob(pattern, recursive=True):
      parser = AdvancedHTMLParser()
      parser.parseFile(html)
      links = parser.getElementsByTagName("a")
      if links:
        self.info("%s: Checking links ...", html)
      else:
        self.debug("%s: No links found...", html)
      for l in links:
        href = l.getAttribute("href")
        if href:
          href = urlparse(href)
          if href.scheme in ["http", "https", "ftp", "ftps" ]:
            try:
              self.info("%s: Checking link to %s...", html, href.geturl())
              r = requests.head(href.geturl(), headers={'user-agent' : "packlint/1.0"}, timeout=10)
              if r.status_code >= 400:
                self.debug(f'HEAD method failed with HTTP-{r.status_code}, falling back to GET method.')
                r = requests.get(href.geturl(), headers={'user-agent': "packlint/1.0"}, timeout=10)
              r.raise_for_status()
            except (requests.exceptions.ConnectionError, requests.exceptions.HTTPError) as e:
              exc_info = None
              if self.loglevel() == logging.DEBUG:
                exc_info = e
              self.warning("%s: Broken web-link to %s!", html, href.geturl(), exc_info=exc_info)
            except requests.exceptions.Timeout as e:
              exc_info = None
              if self.loglevel() == logging.DEBUG:
                exc_info = e
              self.warning("%s: Timeout following web-link to %s.", html, href.geturl(), exc_info=exc_info)
          elif href.scheme == "javascript":
            pass
          elif not os.path.isabs(href.path):
            target = os.path.normpath(os.path.join(os.path.dirname(html), href.path))
            if not os.path.exists(target):
              self.warning("%s: Broken relative-link to %s!", html, href.path)
          else:
            self.warning("%s: Broken relative-link to %s!", html, href.path)
