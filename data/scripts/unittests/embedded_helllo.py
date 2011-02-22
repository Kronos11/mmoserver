#-------------------------------------------------------------------------------
# Name:        test module
# Purpose:
#
# Author:      Kyle Craviotto
#
# Created:     05/02/2011
# Copyright:   (c) SWGANH 2011
# Licence:     <http://www.gnu.org/licenses/>
#-------------------------------------------------------------------------------
#!/usr/bin/env python
from embedded_hello import *
class PythonDerived(Base):
    def hello(self):
        return ('Hello from Python!')
