<?php

/**
 * Provides a search for chado analysis-based Genome Assembly Pages.
 *
 */
class GenomeAssemblySearch extends KPSEARCH {

  /**
   * The human readable title of this search. This will be used in listings
   * and shown on the search page as the title.
   */
  public static $title = 'Genome Assemblies';

  /**
   * The machine name of the module providing this search.
   */
  public static $module = 'kp_searches';

  /**
   * A description of this search. This is shown at the top of the search page
   * and used for the menu item.
   */
  public static $description = '';

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
  public static $num_items_per_page = 50;

  /**
   * This defined the hook_menu definition for this search. At a minimum the
   * path is required.
   */
  public static $menu = [
    'path' => 'genome-assemblies',
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
          'chado_table' => 'analysis',
          'id_column' => 'analysis_id'
        ],
      ],
      'sourcename' => [
        'title' => 'Source Germplasm',
      ],
      'date_released' => [
        'title' => 'Date Released',
      ],
      'stats' => [
        'title' => 'Statistics',
      ],
    ],
    // The filter criteria available to the user.
    // This is used to generate a search form which can be altered.
    'filters' => [
      'genus' => [
        'title' => 'Genus',
        'help' => 'The legume species the genome assembly describes (e.g. Lens culinaris).',
      ],
      'species' => [
        'title' => 'Species',
        'help' => '',
      ],
      'name' => [
        'title' => 'Name',
        'help' => 'The name of the genome assembly you are interested in (partial names are accepted).',
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

    // Add a crop selector.
    $this->addCropFormElement($form, $form_state);

    // Add a Scientific name selector.
    $this->addSpeciesFormElement($form, $form_state);

    // @todo: add species to the database in a consistent manner.
    $form['scientific_name']['species']['#disabled'] = TRUE;
    $form['scientific_name']['genus']['#required'] = FALSE;

    return $form;
  }

  /**
   * Allows custom searches to validate the form results.
   * Use form_set_error() to signal invalid values.
   */
  public function validateForm($form, $form_state) {
    //$values = $form_state['values'];
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
    // @debug dpm($filter_results, '$filter_results');

 
    $query = "
     SELECT
        a.analysis_id, 
        a.name,
        a.description, 
	a.sourcename, 
	a.timeexecuted as date_released, 
        n50.value ||'/'|| l50.value as stats,
        genus.value as genus
     FROM chado.analysis a
     LEFT JOIN chado.analysisprop n50 ON n50.analysis_id=a.analysis_id 
	AND n50.type_id=6773
     LEFT JOIN chado.analysisprop l50 ON l50.analysis_id=a.analysis_id 
        AND l50.type_id=6775
     LEFT JOIN chado.analysisprop genus ON genus.analysis_id=a.analysis_id
        AND genus.type_id=4032";

    $where = [];
    $joins = [];

    $where[] = "a.analysis_id IN (
	SELECT analysis_id FROM chado.analysisprop 
	WHERE type_id=4310 AND value=:content_type)";
    $args[':content_type'] = 'genome_assembly';
    // -- Genus.
    if ($filter_results['genus'] != '') {
      $where[] = "genus.value ~* :genus";
      $args[':genus'] = $filter_results['genus'];
    }

    // -- Species.
    //if ($filter_results['species'] != '') {
    //  $where[] = "species.value ~* :species";
    //  $args[':species'] = $filter_results['species'];
    //}

    // -- Name.
    if ($filter_results['name'] != '') {
      $where[] = "a.name ~ :name";
      $args[':name'] = $filter_results['name'];
    }

    // Finally, add it to the query.
    if (!empty($joins)) {
      $query .= implode("\n",$joins);
    }
    if (!empty($where)) {
      $query .= "\n" . ' WHERE ' . implode(' AND ',$where);
    }

    // Sort even though it is SLOW b/c ppl expect it.
    $query .= "\n" . ' ORDER BY a.timeexecuted DESC, a.name ASC';

    // Handle paging.
    $limit = $this::$num_items_per_page + 1;
    $query .= "\n" . ' LIMIT ' . $limit . ' OFFSET ' . $offset;

    // @debug dpm(strtr(str_replace(['{','}'], ['chado.', ''], $query), $args), 'query');
  }

  /**
   * Format the results within the $form array.
   *
   * The base class will format the results as a table.
   *
   * @param array $form
   *   The current form array.
   * @param array $results
   *   The results to format. This will be an array of standard objects where
   *   the keys map to the keys in $info['fields'].
   */
  public function formatResults(&$form, $results) {
    $list = [];

    foreach ($results as $r) {
      // Add a link to the title.
      $title = $r->name;
      $id = NULL;
      if ($r->analysis_id) {
        $id = chado_get_record_entity_by_table('analysis', $r->analysis_id);
        if ($id) {
          $title = l($r->name, 'bio_data/'.$id);
        }
      }

      // Substring the description.
      $description = strip_tags($r->description);
      if (preg_match('/^.{1,300}\b/s', $description, $match)) {
        $description = trim($match[0]);
      }
      if ($id && (strlen($description) > 0)){
        $description .= ' ' . l('[Read more]', 'bio_data/'.$id);
      }

      $markup = '
        <div class="result-row">
          <div class="result-left">
            <div class="result-title">'.$title.'</div>
            <div class="result-description">'.$description.'</div>
          </div>
	  <div class="result-right">';
      if ($r->date_released) {
        $markup .= '<div class="result-year" title="Year Released">'.date('Y M j', strtotime($r->date_released)).'</div>';
      }
      if ($r->sourcename) {
        $markup .= '<div class="result-genus" title="Source Germplasm">'.$r->sourcename.'</div>';
      }
      if ($r->stats) {
        $markup .= '<div class="result-category" title="Scaffold N50/L50">'.$r->stats.'</div>';
      }
      $markup .= '
          </div>
        </div>';
      $list[] = $markup;
    }

    $form['results'] = [
      '#theme' => 'item_list',
      '#items' => $list,
      '#type' => 'ul',
      '#weight' => 50,
    ];

    return $form;
  }
}
