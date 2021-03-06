<?php

/**
 * @class
 * Provides support for loading of MST map files.
 */
class MSTmapImporter extends TripalImporter {

  /**
   * The name of this loader.
   *
   * This name will be presented to the site user.
   *
   * @var string
   */
  public static $name = 'MSTmap Importer';

  /**
   * The machine name for this loader.
   *
   * This name will be used to construct the URL for the loader.
   *
   * @var string
   */
  public static $machine_name = 'tripal_mstmap_loader';

  /**
   * A brief description for this loader.
   *
   * This description will be presented to the site user.
   *
   * @var string
   */
  public static $description = 'Loads genetic maps (standard MSTmap format).';

  /**
   * An array containing the extensions of allowed file types.
   *
   * @var array
   */
  public static $file_types = ['txt', 'tsv'];

  /**
   * Provides information to the user about the file upload.
   *
   * Typically this may include a description of the file types allowed.
   *
   * @var string
   */
  public static $upload_description = 'MSTmap format consists of a TSV file listing the positions of markers in a genetic map. Each linkage group is labelled with it\'s name followed by a 2-column format: marker name, marker position (centimorgans). For example, <pre>
group lg0
m4  0.000
m2  0.000
m3  0.000
m1  0.000
m8  4.009
m12  4.009
m6  4.009
m7  4.009
m9  5.009
  </pre>';

  /**
   * Indicates the methods that the file uploader will support.
   *
   * @var array
   */
  public static $methods = [
    // Allow the user to upload a file to the server.
    'file_upload' => TRUE,
    // Allow the user to provide the path on the Tripal server for the file.
    'file_local' => TRUE,
    // Allow the user to provide a remote URL for the file.
    'file_remote' => TRUE,
  ];

  /**
   * Do not require an analysis.
   *
   * We create our own analysis record based on information provided and thus
   * do not need to require pre-existing analysis to be linked.
   *
   * @var bool
   */
  public static $use_analysis = FALSE;

  /**
   * Provides additional elements for the upload form including map metadata.
   */
  public function form($form, &$form_state) {

    // Map Name.
    $form['featuremap_name'] = [
      '#type' => 'textfield',
      '#title' => 'Map Name',
      '#required' => TRUE,
      '#description' => 'A unique canonical name for the linkage map as defined by the curator.',
    ];

    $form['published_map_name'] = [
      '#type' => 'textfield',
      '#title' => 'Published Map Name',
      '#description' => 'The name of the map as it was published.',
    ];

    // Organism.
    $form['organism_organism_id'] = [
      '#type' => 'select',
      '#title' => 'Map Species',
      '#required' => TRUE,
      '#description' => "The species the map should be catagorized under based on the parental species. Choose interspecific if the parental species are different.",
      '#options' => chado_get_organism_select_options(FALSE),
    ];

    // Units (cM default).
    $form['featuremap_unittype_name'] = [
      '#type' => 'hidden',
      '#title' => 'Map Units',
      '#required' => TRUE,
      '#description' => 'The units the current map is in.',
      '#value' => 'cM',
    ];

    // Map Type (genetic, physical, cytogenetic, sequence).
    $form['map_type'] = [
      '#type' => 'select',
      '#title' => 'Map Type',
      '#required' => TRUE,
      '#description' => 'The type of map.',
      '#options' => [
        'genetic' => 'genetic',
        'physical' => 'physical',
        'cytogenetic' => 'cytogenetic',
        'sequence' => 'sequence',
      ],
    ];

    // Population Type (e.g. F2, BC1, RIL, ABC, etc.
    $form['population_type'] = [
      '#type' => 'textfield',
      '#title' => 'Population Type',
      '#description' => 'The type of population used to generate the map (e.g. F2, BC1, RIL, ABC).',
    ];

    // Population Size.
    $form['population_size'] = [
      '#type' => 'textfield',
      '#title' => 'Population Size',
      '#description' => 'The number of individuals in the population used to generate this map.',
    ];

    // Population.
    // Use a stock collection?

    // Parent 1.

    // Parent 2.

    // Contact Person.

    // Publication (if available).

    // Analysis Method.
    // Software for map construction (e.g MSTmap).
    $form['analysis_program'] = [
      '#type' => 'textfield',
      '#title' => 'Mapping Software',
      '#required' => TRUE,
      '#description' => 'The name of the mapping software used to generate the map (e.g. MSTmap, MapMaker EXP, MapMaker Macintosh, Mapdisto, CarthaGene, MergeMap, QTL IciMapping, JoinMap)',
    ];

    // Software version.
    $form['analysis_programversion'] = [
      '#type' => 'textfield',
      '#title' => 'Mapping Software Version',
      '#required' => TRUE,
      '#description' => 'The version of the above mapping software which was used.',
    ];

    // Analysis sourcename.
    $form['analysis_sourcename'] = [
      '#type' => 'textfield',
      '#title' => 'Data Source',
      '#required' => TRUE,
      '#description' => 'A unique name for the data you used to generate this map (e.g. "RIL### Exome Capture Jun 2019").',
    ];

    // Analysis Description.
    $form['analysis_description'] = [
      '#type' => 'textarea',
      '#title' => 'Methodology',
      '#description' => 'Description of how the linkage map was created. Specifically, include software parameters, whether the markers were binned and describe any manual manipulation of the map.',
    ];

    // Map Description.
    $form['featuremap_description'] = [
      '#type' => 'textarea',
      '#title' => 'Map Description',
      '#description' => "Description of the linkage map.  Suggested topics include the published source; alternative names for the map; types of molecular markers; parents, size, and type of mapping population; linkage group number, length, marker count, and sub-genome assignment; the map's total length and marker count; average inter-marker gap distance or marker density.  Indicate if there are PCR primers that amplify more than a single locus on the map, if there is a framework map available in addition to the full genetic map, and compare and contrast with other genetic maps.  Describe whether linkage groups are homeologues and to what extent common markers and conserved marker orders are present.  Record if the map was produced directly using a bi-parental mapping population or whether it is a consensus or composite map; if it is the latter then list and describe briefly the constituent mapping populations used in its construction.  Describe how many markers were screened against the parents, what number were polymorphic and used for population genotyping, and what proportion appeared on the final map.",
    ];

    return $form;
  }

  /**
   * Validate the form input.
   */
  public function formValidate($form, &$form_state) {}

  /**
   * Import the map.
   */
  public function run() {

    // All values provided by the user in the Importer's form widgets are
    // made available to us here by the Class' arguments member variable.
    $arguments = $this->arguments['run_args'];
    print "Arguements supplied include:\n";
    foreach ($arguments as $k => $v) {
      print "\t-$k: $v\n";
    }

    // The path to the uploaded file is always made available using the
    // 'files' argument. The importer can support multiple files, therefore
    // this is an array of files, where each has a 'file_path' key specifying
    // where the file is located on the server.
    $file_path = $this->arguments['files'][0]['file_path'];

    // Save map meta-data.
    print "\nSaving metadata...\n";
    $featuremap_id = $this->loadMapMetadata($arguments);
    if (!$featuremap_id) {
      $this->logMessage('An Error was encountered importing the map metadata.');
    }

    // Save Map positions (file).
    print "\nSaving map locations...\n";
    $success = $this->loadMapFile($file_path, $featuremap_id, $arguments);

    if ($success) {
      $this->logMessage('The map was imported successfully.');
    }
    else {
      $this->logMessage('An Error was encountered importing the map file.');
    }
    return $success;
  }

  /**
   * Creater featuremap and featuremapprop records.
   */
  public function loadMapMetadata($args) {
    $debug = FALSE;

    $feature_map = chado_insert_record('featuremap', [
      'name' => $args['featuremap_name'],
      'description' => $args['featuremap_description'],
      'unittype_id' => ['name' => $args['featuremap_unittype_name']],
    ]);

    if (!empty($feature_map)) {
      $mapid = $feature_map['featuremap_id'];
      if ($debug) { print "\tCreated map record (ID=$mapid).\n"; }

      // Create Analysis.
      $analysis = chado_insert_record('analysis', [
        'name' => 'Generate ' . $args['featuremap_name'],
        'program' => $args['analysis_program'],
        'programversion' => $args['analysis_programversion'],
        'sourcename' => $args['analysis_sourcename'],
        'description' => $args['analysis_description'],
      ]);
      if (empty($analysis)) {
        $this->logMessage('ERROR: Unable to insert analysis record.');
        return FALSE;
      }
      if ($debug) { print "\tCreated analysis (ID=".$analysis['analysis_id'].").\n"; }

      // Link the analysis to the map.
      chado_insert_record('featuremap_analysis', [
        'analysis_id' => $analysis['analysis_id'],
        'featuremap_id' => $mapid,
      ]);

      // Link map to the organism.
      chado_insert_record('featuremap_organism', [
        'featuremap_id' => $mapid,
        'organism_id' => $args['organism_organism_id'],
      ]);

      // Create properties of the map.
      $property_types = [
        'map_type', 'published_map_name', 'population_size', 'population_type',
      ];
      foreach ($property_types as $type_name) {
        chado_insert_property(
          ['table' => 'featuremap', 'id' => $mapid],
          [
            'type_name' => $type_name,
            'cv_name' => 'MAIN',
            'value' => $args[$type_name],
          ]
        );
      }

      // Also insert the type as rdfs:type for Tripal Content type use.
      chado_insert_property(
        ['table' => 'featuremap', 'id' => $mapid],
        [
          'type_name' => 'type',
          'cv_name' => 'rdfs',
          'value' => $args['map_type'],
        ]
      );

      return $mapid;
    }
    else {
      $this->logMessage('ERROR: Unable to insert featuremap record.');
      return FALSE;
    }
  }

  /**
   * Load the map file.
   */
  public function loadMapFile($file_path, $mapid, $args) {
    $debug = FALSE;

    // We want to provide a progress report to the end-user so that they:
    // 1) Recognize that the loader is not hung if running a large file.
    // 2) Provides some indicatation for how long the file will take to load.
    //
    // Here we'll get the size of the file and tell the TripalImporter how
    // many "items" we have to process (in this case bytes of the file).
    $filesize = filesize($file_path);
    $this->setTotalItems($filesize);
    $this->setItemsHandled(0);

    // Loop through each line of file.  We use the fgets function so as not
    // to load the entire file into memory but rather to iterate over each
    // line separately.
    $bytes_read = 0;
    $in_fh = fopen($file_path, "r");
    while ($line = fgets($in_fh)) {

      // Calculate how many bytes we have read from the file and let the
      // importer know how many have been processed so it can provide a
      // progress indicator.
      $bytes_read += drupal_strlen($line);
      $this->setItemsHandled($bytes_read);

      // Remove any trailing white-space from the line.
      $line = trim($line);

      // Skip empty lines.
      if (empty($line)) {
        continue;
      }
      // Skip Comments.
      if ($line[0] == ';') {
        continue;
      }

      // If we are looking at a new linkage group...
      // If the line begins with `group`.
      if (strpos($line, 'group') === 0) {
        $linkage_group_name = trim(str_ireplace('group', '', $line));
        if ($debug) { print "\tCreating LG '$linkage_group_name'.\n";}

        // Insert the linkage group.
        $values = [
          'uniquename' => $linkage_group_name . ' from map' . $mapid,
          'name' => $linkage_group_name,
          'organism_id' => $args['organism_organism_id'],
          'type_id' => ['name' => 'linkage_group', 'cv_id' => ['name' => 'sequence']],
        ];
        $linkage_group = chado_select_record('feature', ['*'], $values);
        if (empty($linkage_group)) {
          $linkage_group = chado_insert_record('feature', $values);
          $linkage_group_id = $linkage_group['feature_id'];
        }
        else {
          $linkage_group_id = $linkage_group[0]->feature_id;
        }
      }
      else {

        // Split line on a tab into an array.
        $cols = explode("\t", $line);
        $locus_name = $cols[0];
        $marker_name = $locus_name;
        $position = $cols[1];

        // Insert the locus.
        $values = [
          'uniquename' => $locus_name,
          'name' => $locus_name,
          'organism_id' => $args['organism_organism_id'],
          'type_id' => ['name' => 'marker_locus', 'cv_id' => ['name' => 'MAIN']],
        ];
        $locus = chado_select_record('feature', ['*'], $values);
        if (!$locus) {
          $locus = chado_insert_record('feature', $values);
          $locus_id = $locus['feature_id'];
        }
        else {
          $locus_id = $locus[0]->feature_id;
        }
        if ($debug) { print "\tCreated LOCUS '$locus_id'.\n";}

        // Look up the genetic marker.
        // @todo only allow select once we have a marker importer.
        $values = [
          'name' => $marker_name,
          'organism_id' => $args['organism_organism_id'],
          'type_id' => ['name' => 'genetic_marker', 'cv_id' => ['name' => 'sequence']],
        ];
        $marker = chado_select_record('feature', ['*'], $values);
        if (!$marker) {
          $values['uniquename'] = $marker_name;
          $marker = chado_insert_record('feature', $values);
          $marker_id = $marker['feature_id'];
        }
        else {
          $marker_id = $marker[0]->feature_id;
        }
        if ($debug) { print "\tCreated Marker '$marker_id'.\n";}

        // Also insert the marker type.
        // @todo remove this in the future in favour of a marker importer.
        chado_insert_property(
          ['table' => 'feature', 'id' => $marker_id],
          [
            'type_name' => 'marker_type',
            'cv_name' => 'MAIN',
            'value' => 'SNP',
          ]
        );

        // Link the locus and the genetic marker.
        $values = [
          'subject_id' => $locus_id,
          'type_id' => ['name' => 'instance_of'],
          'object_id' => $marker_id,
        ];
        $fr = chado_select_record('feature_relationship', ['*'], $values);
        if (!$fr) {
          $fr = chado_insert_record('feature_relationship', $values);
        }

        // Create the featurepos record.
        $values = [
          'featuremap_id' => $mapid,
          'map_feature_id' => $linkage_group_id,
          'feature_id' => $locus_id,
          'mappos' => $position,
        ];
        $pos = chado_select_record('featurepos', ['*'], $values);
        if (!$pos) {
          $pos = chado_insert_record('featurepos', $values);
          $pos_id = $pos['featurepos_id'];
        }
        if ($debug) { print "\tCreated Position '$pos_id'.\n";}

        // Attach the position to the featurepos record.
        $values = [
          'featurepos_id' => $pos_id,
          'type_id' => ['name' => 'start', 'cv_id' => ['name' => 'MAIN']],
          'value' => $position,
        ];
        $posprop = chado_select_record('featureposprop', ['*'], $values);
        if (!$posprop) {
          $posprop = chado_insert_record('featureposprop', $values);
        }

      }
    }

    return TRUE;
  }

}
