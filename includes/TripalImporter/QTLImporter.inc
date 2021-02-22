<?php

/**
 * @class
 * Provides support for loading of MST map files.
 */
class QTLImporter extends TripalImporter {
  
  /**
   * The name of this loader.
   *
   * This name will be presented to the site user.
   *
   * @var string
   */
  public static $name = 'QTL Importer';

  /**
   * The machine name for this loader.
   *
   * This name will be used to construct the URL for the loader.
   *
   * @var string
   */
  public static $machine_name = 'tripal_qtl_loader';

  /**
   * A brief description for this loader.
   *
   * This description will be presented to the site user.
   *
   * @var string
   */
  public static $description = 'Loads quantitative trait loci (QTL) data.';

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
  public static $upload_description = '<p>Since QTL analysis stoftware does not output in a common format, we have compiled the key information into a simple tab-delimited format:</p>
  <ol>
    <li><strong>QTL Published Name</strong>: The name of the QTL as it was published.</li>
    <li><strong>QTL Database Name</strong>: A unique name for use within this data repository. This should follow the format <pre>[trait abbreviation][linkage group].[qtl number].[maternal]x[paternal].[siteyear]</pre> For example, "SDDIA1.2.CDCRobinx964a-46.Pres2009" would be the second QTL for Seed Diameter on linkage group 1 in the CDC Robin x 964a-46 population.</li>
    <li><strong>Trait Full Name</strong>: The full name of the trait the QTL is contributing to (e.g. Days to Flower).</li>
    <li><strong>Trait Abbreviation</strong>: A short 2-5 letter abbreviation of the trait (e.g. DTF).</li>
    <li><strong>Environments (SiteYears)</strong>: The environments a QTL is significant in (e.g. Preston 2009).</li>
    <li><strong>Peak Marker</strong>: The genetic marker closest to the peak LOD of the QTL.</li>
    <li><strong>Linkage Group</strong>: The linkage group the QTL is located on.</li>
    <li><strong>Peak Position</strong>: The cM position of the peak LOD for the QTL.</li>
    <li><strong>Peak LOD</strong>: The highest LOD score on the QTL graph. </li>
    <li><strong>Phenotypic r2</strong>: Phenotypic variance explained by the QTL (expressed as a fraction).</li>
    <li><strong>Additive Effect</strong>: The additive effect of each allele expressed as a fraction. The parent contributing the allele is supplied in the next column. This value is usually supplied as a negative or positive value dependant upon the contributing parent.</li>
    <li><strong>Contributor Parent</strong>: The parent which contributes to the effect of the QTL based on the value of the additive effect.</li>
    <li><strong>Confidence Interval Left (1 LOD drop)</strong>:The LEFT cM position on the QTL graph where the LOD is ONE less then the peak LOD.</li>
    <li><strong>Confidence Interval Right (1 LOD drop)</strong>: The RIGHT cM position on the QTL graph where the LOD is ONE less then the peak LOD.</li>
    <li><strong>Confidence Interval Left (2 LOD drop)</strong>: The LEFT cM position on the QTL graph where the LOD is TWO less then the peak LOD. </li>
    <li><strong>Confidence Interval Right (2 LOD drop)</strong>: The RIGHT cM position on the QTL graph where the LOD is TWO less then the peak LOD.</li>
  </ol>
  <p>NOTE: Column order is important and your file is expected to contain a header.</p>';

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
    $options = chado_query('SELECT featuremap_id, name  FROM {featuremap}')->fetchAllKeyed(0,1);
    $form['featuremap_name'] = [
      '#type' => 'select',
      '#title' => 'Map Name',
      '#required' => TRUE,
      '#options' => $options,
      '#empty_option' => ' - Select Map -',
      '#description' => 'The genetic map QTL analysis was done on. This map must already be imported.',
      '#weight' => -100
    ];

    $options = chado_query('SELECT cv_id, name  FROM {cv} ORDER BY name ASC')->fetchAllKeyed(0,1);
    $form['trait_cv_id'] = [
      '#type' => 'select',
      '#title' => 'Trait Vocabulary',
      '#options' => $options,
      '#empty_option' => ' - Select Vocab -',
      '#description' => 'All traits associated within your file are expected to already exist. Specifically, the "Trait Full Name" must match exactly and the be in the selected vocabulary.',
      '#weight' => -90
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
  public function run($f_id = null) {

    // All values provided by the user in the Importer's form widgets are
    // made available to us here by the Class' arguments member variable.
    $arguments = $this->arguments['run_args'];
    print "\nArguments supplied include:\n";
    foreach ($arguments as $k => $v) {
      print "\t-$k: $v\n";
    }

    // The path to the uploaded file is always made available using the
    // 'files' argument. The importer can support multiple files, therefore
    // this is an array of files, where each has a 'file_path' key specifying
    // where the file is located on the server.
    $file_path = $this->arguments['files'][0]['file_path'];

    // Retrieve map to add QTL Data to.
    $featuremap_id = ($f_id && $f_id > 1) ? $f_id : $arguments['featuremap_name'];

    if (!$featuremap_id) {
      $this->logMessage('An Error was encountered retrieving the genetic map.');
      return FALSE;
    }
    // Check that the map exists.
    $exists = chado_query('SELECT true FROM {featuremap} WHERE
      featuremap_id=:id', [':id' => $featuremap_id])->fetchField();
    if (!$exists) {
      $this->logMessage('An Error was encountered retrieving the genetic map.');
      return FALSE;
    }

    // Retrieve trait to add QTL Data to.
    $trait_cv_id = $arguments['trait_cv_id'];
    if (!$trait_cv_id) {
      $this->logMessage('An Error was encountered retrieving the trait cv.');
      return FALSE;
    }
    // Check that the trait exists.
    $exists = chado_query('SELECT true FROM {cv} WHERE
      cv_id=:id', [':id' => $trait_cv_id])->fetchField();
    if (!$exists) {
      $this->logMessage('An Error was encountered retrieving the trait.');
      return FALSE;
    }

    // Next, add the QTLs.
    $success = $this->importQTLsfromFile($file_path, $featuremap_id, $trait_cv_id);

    //Finally, report completion to the user.
    if ($success) {
      $this->logMessage('The map was imported successfully.');
    }
    else {
      $this->logMessage('An Error was encountered importing the QTL data file.');
    }
    return $success;
  }

  /**
   * Import the QTL into the database.
   *
   * @param $file_path
   *   The file containing QTLs to be imported.
   * @param $feature_map_id
   *   The ID of the genetic map the QTL are from.
   * @return
   *   TRUE or BALSE depending on successful import.
   */
  public function importQTLsfromFile($file_path, $feature_map_id, $trait_cv_id) {
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

    // Remove header.
    $have_header = FALSE;
    while (!$have_header) {
      $line = fgets($in_fh);

      // Skip Comments.
      if ($line[0] == ';') {
        continue;
      }
      // Otherwise, it's our header?
      else {
        $have_header = TRUE;
      }
    }

    // Now process each data containing line.
    while ($line = fgets($in_fh)) {

      // Calculate how many bytes we have read from the file and let the
      // importer know how many have been processed so it can provide a
      // progress indicator.
      $bytes_read += drupal_strlen($line);
      $this->setItemsHandled($bytes_read);

      // Skip empty lines.
      if (empty(trim($line))) {
        continue;
      }
      // Skip Comments.
      if ($line[0] == ';') {
        continue;
      }

      // Remove any trailing white-space from the line.
      $linetmp = explode("\t", $line);
      // Make an associative array for more readable code.
      $line = [
        'published_symbol' => trim($linetmp[0]),
        'trait_name' => trim($linetmp[1]),
        'trait_symbol' => trim($linetmp[2]),
        'siteyear' => trim($linetmp[3]),
        'peak_marker' => trim($linetmp[4]),
        'linkage_group' => trim($linetmp[5]),
        'peak_pos' => trim($linetmp[6]),
        'peak_lod' => trim($linetmp[7]),
        'r2' => trim($linetmp[8]),
        'addt_effect' => trim($linetmp[9]),
        'addt_parent' => trim($linetmp[10]),
        'CIstart' => trim($linetmp[11]),
        'CIend' => trim($linetmp[12]),
        'CIlod2start' => trim($linetmp[13]),
        'CIlod2end' => trim($linetmp[14]),
      ];
      if ($debug) { print_r($line); }

      // Lookup Linkage group. Must already exist.
      $linkage_group = [
        'name' => $line['linkage_group'],
        'uniquename' => $line['linkage_group'] . ' from map' . $feature_map_id,
        'type_id' => ['name' => 'linkage_group'],
      ];
      $linkage_group_id = $this->lookup_id('feature',$linkage_group, 'feature_id');
      if (!$linkage_group_id) {
        print "\n".'Error: Unable to lookup existing linkage group "'.$line['linkage_group'].'".';
        $this->logMessage('Error: Unable to lookup existing linkage group "'.$line['linkage_group'].'".');
        return FALSE;
      }

      // Look up the organism based on the linkage group.
      $organism_id = chado_query('SELECT organism_id FROM {feature} WHERE feature_id=:id',
        [':id' => $linkage_group_id])->fetchField();


      // Generate uniquename for QTL.
      // Tripal map assumes a non-important leading character (e.g. q)
      // then displays everything to the first period. We want to display
      // the trait symbol.
      // q[trait symbol].[linkage group]-[peak position].map[map ID].[siteyear]
      $qtl_dbname = 'q' . $line['trait_symbol'] . '.'
        . $line['linkage_group'] . '-' . $line['peak_pos']
        . '.map' . $feature_map_id . '.' . $line['siteyear'];

      // Create QTL.
      $QTL = [
        'name' => $line['published_symbol'],
        'uniquename' => $qtl_dbname,
        'organism_id' => $organism_id,
        'type_id' => ['name' => 'QTL', 'cv_id' => ['name' => 'sequence']],
      ];
      $QTL_id = $this->save_datapoint('feature', $QTL, 'feature_id');

      // Add Metadata as properties of the QTL.
      // -- Trait Abbreviation.
      $this->save_feature_property($QTL_id, 'MAIN', 'published_symbol', $line['trait_symbol']);
      // -- Environment.
      $this->save_feature_property($QTL_id, 'MAIN', 'site_name', $line['siteyear']);
      // -- Peak LOD.
      $this->save_feature_property($QTL_id, 'MAIN', 'lod', $line['peak_lod']);
      // -- r2.
      $this->save_feature_property($QTL_id, 'MAIN', 'r2', $line['r2']);
      // -- Additive Effect.
      $this->save_feature_property($QTL_id, 'MAIN', 'additive_effect', $line['addt_effect']);
      // -- Contributor Parent.
      $this->save_feature_property($QTL_id, 'MAIN', 'direction', $line['addt_parent']);

      // Associate Peak Marker.
      $this->save_feature_property($QTL_id, 'MAIN', 'marker_locus', $line['peak_marker']);

      // Lookup Trait.
      $trait = [
        'name' => $line['trait_name'],
        'cv_id' => $trait_cv_id,
      ];
      $trait_id = $this->lookup_id('cvterm',$trait, 'cvterm_id');
      if (!$trait_id) {
        $message = 'Error: Unable to lookup existing trait "'.$line['trait_name'].'".';
        print "\n$message.";
        $this->logMessage($message);
        return FALSE;
      }

      // -- Link to Trait.
      $trait_link = [
        'feature_id' => $QTL_id,
        'cvterm_id' => $trait_id,
        'pub_id' => 1,
      ];
      $this->save_datapoint('feature_cvterm', $trait_link, 'feature_cvterm_id');

      // Add the positions.
      // -- Peak Position.
      $pos = [
        'featuremap_id' => $feature_map_id,
        'map_feature_id' => $linkage_group_id,
        'feature_id' => $QTL_id,
        'mappos' => $line['peak_pos'],
      ];
      $featurepos_id = $this->save_datapoint('featurepos', $pos, 'featurepos_id');
      $this->save_feature_property($featurepos_id, 'MAIN', 'qtl_peak', $line['peak_pos'], $table = 'featurepos');

      // -- CI 1 LOD drop.
      if ($line['CIstart']) {
        $this->save_feature_property($featurepos_id, 'MAIN', 'start', $line['CIstart'], $table = 'featurepos');
        $this->save_feature_property($featurepos_id, 'MAIN', 'end', $line['CIend'], $table = 'featurepos');
      }

      // -- CI 2 LOD drop: start.
      // -- CI 2 LOD drop: end.
      // @todo
    }

    return TRUE;
  }

  /**
   * HELPER: Insert-Update feature properties!
   */
  public function save_feature_property($id, $cv_name, $cvterm_name, $value, $table = 'feature') {

    $record_values = [
      'table' => $table,
      'id' => $id,
    ];
    $property_values = [
      'cv_name' => $cv_name,
      'type_name' => $cvterm_name,
      'value' => $value,
    ];
    $property = chado_get_property($record_values, $property_values);

    // Insert it if it doesn't already exist.
    if (!$property) {
      return chado_insert_property($record_values, $property_values);
    }
    else {
      return chado_update_property($record_values, $property_values);
    }
  }

  /**
   * HELPER: Provides lookup functionality for this importer.
   */
  public function lookup_id($chado_table, $values, $id_column) {
    $record = chado_select_record($chado_table, ['*'], $values);
    if ($record) {
      return $record[0]->{$id_column};
    }
    else {
      return FALSE;
    }
  }

  /**
   * HELPER: Provides Insert or Update functionality for this importer.
   */
  public function save_datapoint($chado_table, $values, $id_column) {

    $record = chado_select_record($chado_table, ['*'], $values);
    if (empty($record)) {
      $record = chado_insert_record($chado_table, $values);
      return $record[$id_column];
    }
    else {
      return $record[0]->{$id_column};
    }
  }
}
