
Installation
============

Dependencies
------------

 - `Tripal 3.x <https://drupal.org/project/tripal>`_

Installation
-------------

The preferred method of installation is using Drush:

.. code:: bash

  cd [your drupal root]/sites/all/modules
  git clone https://github.com/UofS-Pulse-Binfo/tripal_qtl.git

The above command downloads the module into the expected directory (e.g. /var/www/html/sites/all/modules/tripal_qtl). Next we need to install the module:

.. code:: bash

  drush pm-enable tripal_qtl

That's it! No configuration needed!
