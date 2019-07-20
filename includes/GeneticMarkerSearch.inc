<?php

/**
 * Provides a search for SO:genetic_marker chado feature-based Tripal Content.
 *
 */
class GeneticMarkerSearch extends ChadoCustomSearch {

  /**
   * The human readable title of this search. This will be used in listings
   * and shown on the search page as the title.
   */
  public static $title = 'Genetic Marker Search';

  /**
   * The machine name of the module providing this search.
   */
  public static $module = 'kp_searches';

  /**
   * A description of this search. This is shown at the top of the search page
   * and used for the menu item.
   */
  public static $description = 'While the term variant refers to a position in the genome with variation, genetic marker refers specifically to an assay which samples that variation.';

  /**
   * The machine names of the permissions with access to this search. This is
   * used to map your search to existing permissions. It must be an array and
   * is used in hook_menu(). Some examples include 'access content'
   * and 'administer tripal'.
   */
  public static $permissions = ['access content'];

  /**
   * If TRUE, this search will require the submit button to be clicked before
   * executing the query; whereas, if FALSE it will be executed on the
   * first page load without parameters.
   *
   * NOTE: to control the results without parameters check $this->submitted
   * in getQuery() and if FALSE return your pre-submit query.
   */
  public static $require_submit = FALSE;

  /**
   * Add a pager to search results
   * with $num_items_per_page results on a single page.
   * NOTE: Your query has to handle paging.
   */
  public static $pager = TRUE;
  public static $num_items_per_page = 25;

  /**
   * This defined the hook_menu definition for this search. At a minimum the
   * path is required.
   */
  public static $menu = [
    'path' => 'search/markers',
    // @todo support menu items.
  ];

  /**
   * Add CSS/JS to the form/results page.
   * NOTE: paths supplied should be relative to $module.
   */
  public static $attached = [
    'css' => [
      'css/all_kp_searches.css',
    ],
    'js' => [],
  ];

  /**
   * Information regarding the fields and filters for this search.
   */
  public static $info = [
    // Lists the columns in your results table.
    'fields' => [
      'name' => [
        'title' => 'Name',
        'entity_link' => [
          'chado_table' => 'feature',
          'id_column' => 'feature_id'
        ],
      ],
      'type' => [
        'title' => 'Marker Type',
      ],
      'species' => [
        'title' => 'Source Species',
        'entity_link' => [
          'chado_table' => 'organism',
          'id_column' => 'organism_id'
        ],
      ],
    ],
    // The filter criteria available to the user.
    // This is used to generate a search form which can be altered.
    'filters' => [
      'species' => [
        'title' => 'Source Species',
        'help' => 'The legume species the marker was developed for (e.g. Lens culinaris).',
      ],
      'type' => [
        'title' => 'Marker Type',
        'help' => 'The type of marker you are interested in (e.g. Illumina Golden Gate).',
        'default' => 'Exome Capture',
      ],
      'project' => [
        'title' => 'Experiment',
        'help' => 'The experiment you are interested in (e.g. AGILE; partial names are accepted).',
      ],
      'name' => [
        'title' => 'Name',
        'help' => 'The name or accession of the marker (e.g. LcC00002p390; partial names are accepted).',
      ],
    ],
  ];

  /**
   * Text that should appear on the button at the bottom of the importer
   * form.
   */
  public static $button_text = 'Search';

  /**
   * Generate the filter form.
   *
   * The base class will generate textfields for each filter defined in $info,
   * set defaults, labels and descriptions, as well as, create the search
   * button.
   *
   * Extend this method to alter the filter form.
   */
  public function form($form, $form_state) {
    $form = parent::form($form, $form_state);

    // Make the species a drop-down.
    $options = chado_query("SELECT genus||' '||species
      FROM {organism}")->fetchAllKeyed(0,0);
    $form['species']['#type'] = 'select';
    $form['species']['#options'] = $options;
    $form['species']['#empty_option'] = ' - Select -';

    // Make the marker type a drop-down.
    $query = "WITH RECURSIVE t AS (
      (SELECT value
        FROM {featureprop}
        ORDER BY value
        LIMIT 1)
      UNION ALL
      SELECT
          (SELECT value
            FROM {featureprop}
            WHERE value > t.value
            ORDER BY value
            LIMIT 1)
        FROM t
        WHERE t.value IS NOT NULL
    )
    SELECT t.value FROM t WHERE t.value IS NOT NULL";
    $options = chado_query($query)->fetchAllKeyed(0,0);
    $form['type']['#type'] = 'select';
    $form['type']['#options'] = $options;
    $form['type']['#empty_option'] = ' - Select -';

    // Make the project an autocomplete.
    // DEPENDENCY: nd_genotypes.
    $form['project']['#autocomplete_path'] = 'tripal_ajax/nd_genotypes/project/name';

    return $form;
  }

  /**
   * Determine the query for the genetic marker search.
   *
   * @param string $query
   *   The full SQL query to execute. This will be executed using chado_query()
   *   so use curly brackets appropriately. Use :placeholders for any values.
   * @param array $args
   *   An array of arguments to pass to chado_query(). Keys must be the
   *   placeholders in the query and values should be what you want them set to.
   */
  public function getQuery(&$query, &$args, $offset) {

    // Retrieve the filter results already set.
    $filter_results = $this->values;

    $query = "
      SELECT marker.feature_id, marker.name as name, type.value as type, o.organism_id, o.genus||' '||o.species as species
      FROM {feature} marker
      LEFT JOIN {organism} o ON o.organism_id=marker.organism_id
      LEFT JOIN {featureprop} type ON type.feature_id = marker.feature_id
      LEFT JOIN {project_feature} pf ON pf.feature_id=marker.feature_id
      LEFT JOIN {project} project ON project.project_id=pf.project_id
        AND type.type_id IN
          (SELECT cvterm_id FROM chado.cvterm WHERE name='additionalType')";

    $where = [];
    // -- Species.
    if ($filter_results['species'] != '') {
      $where[] = "o.genus||' '||o.species ~ :species";
      $args[':species'] = $filter_results['species'];
    }

    // -- Type.
    if ($filter_results['type'] != '') {
      $where[] = "type.value ~ :type";
      $args[':type'] = $filter_results['type'];
    }

    // -- Project.
    if ($filter_results['project'] != '') {
      $where[] = "project.name ~ :project";
      $args[':project'] = $filter_results['project'];
    }

    // -- Name.
    if ($filter_results['name'] != '') {
      $where[] = "(marker.name ~ :name OR marker.uniquename = :name)";
      $args[':name'] = $filter_results['name'];
    }

    // Finally, add it to the query.
    if (!empty($where)) {
      $query .= ' WHERE ' . implode(' AND ',$where);
    }

    // Sort even though it is SLOW b/c ppl expect it.
    $query .= ' ORDER BY marker.name ASC';

    // Handle paging.
    $limit = $this::$num_items_per_page + 1;
    $query .= ' LIMIT ' . $limit . ' OFFSET ' . $offset;
  }
}
