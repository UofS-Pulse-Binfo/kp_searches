<?php
/**
 * Provides a search specific to breeding crosses.
 */
class RILSearch extends GermplasmSearch {

  /**
   * The human readable title of this search. This will be used in listings
   * and shown on the search page as the title.
   */
  public static $title = 'Recombinant Inbred Line (RIL) Search';

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
  public static $permissions = ['view bio_data_29'];

  /**
   * If TRUE, this search will require the submit button to be clicked before
   * executing the query; whereas, if FALSE it will be executed on the
   * first page load without parameters.
   *
   * NOTE: to control the results without parameters check $this->submitted
   * in getQuery() and if FALSE return your pre-submit query.
   */
  public static $require_submit = TRUE;

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
    'path' => 'search/germplasm/RILs',
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
          'chado_table' => 'stock',
          'id_column' => 'stock_id'
        ],
      ],
      'mom' => [
        'title' => 'Maternal Parent',
        'entity_link' => [
          'chado_table' => 'stock',
          'id_column' => 'mom_id'
        ],
      ],
      'mom_species' => [
        'title' => 'Species',
      ],
      'dad' => [
        'title' => 'Paternal Parent',
        'entity_link' => [
          'chado_table' => 'stock',
          'id_column' => 'dad_id'
        ],
      ],
      'dad_species' => [
        'title' => 'Species',
      ],
    ],
    // The filter criteria available to the user.
    // This is used to generate a search form which can be altered.
    'filters' => [
      'genus' => [
        'title' => 'Genus',
        'help' => 'The legume genus the germplasm belongs to (e.g. Lens).',
      ],
      'species' => [
        'title' => 'Species',
        'help' => 'The legume species the germplasm belongs to (e.g. culinaris).',
      ],
      'mom' => [
        'title' => 'Maternal Parent',
        'help' => 'The maternal parent of the germplasm you are interested in.'
      ],
      'dad' => [
        'title' => 'Paternal Parent',
        'help' => 'The paternal parent of the germplasm you are interested in.'
      ],
      'reciprocal_crosses' => [
        'title' => 'Include reciprocal crosses',
        'help' => 'Includes both A x B and B x A crosses when using the parentage filter.',
      ],
      'show_individiuals' => [
        'title' => 'Show RIL Individuals',
        'help' => 'When selected, search results will include RIL individuals (e.g. LR-18-25).'
      ],
      'name' => [
        'title' => 'Name',
        'help' => 'The name or accession of the germplasm (e.g. CDC Redberry; partial names are accepted).',
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
    $form = ChadoCustomSearch::form($form, $form_state);

    // Add a crop selector.
    $this->addCropFormElement($form, $form_state);

    // Add a Scientific name selector.
    $this->addSpeciesFormElement($form, $form_state);

    // Add a Maternal/paternal name selector.
    $this->addparentageFormElement($form, $form_state);

    // Change show individuals to a checkbox.
    $form['show_individiuals']['#type'] = 'checkbox';
    $form['show_individiuals']['#weight'] = 11;

    // Move name to the bottom.
    $form['name']['#weight'] = 10;

    return $form;
  }
  /**
   * Allows custom searches to validate the form results.
   * Use form_set_error() to signal invalid values.
   */
  public function validateForm($form, $form_state) { }

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
    global $user;

    // Retrieve the filter results already set.
    $filter_results = $this->values;
    // @debug dpm($filter_results, '$filter_results');

    // To get the parents...
    // 1) Cultivar -is_registered_cultivar_of-> individual -is_selection_of-> cross
    // 2) Cultivar -is_registered_cultivar_of-> cross
    // type_id: is_registered_cultivar_of = 3679; is_selection_of = 3684;
    //    source_institute = 3711
    $query = "SELECT
        s.name,
        s.uniquename,
        s.stock_id,
        mom.name as mom,
        mom.stock_id as mom_id,
        momo.species as mom_species,
        dad.name as dad,
        dad.stock_id as dad_id,
        dado.species as dad_species
      FROM {stock} s
      LEFT JOIN {organism} o ON o.organism_id=s.organism_id
      LEFT JOIN {stock_relationship} origcross ON origcross.subject_id=s.stock_id
        AND origcross.type_id = 3684
      LEFT JOIN {stock_relationship} momr ON momr.object_id=origcross.object_id
        AND momr.type_id=3632
      LEFT JOIN {stock_relationship} dadr ON dadr.object_id=origcross.object_id
        AND dadr.type_id=3633
      LEFT JOIN {stock} mom ON momr.subject_id=mom.stock_id
      LEFT JOIN {stock} dad ON dadr.subject_id=dad.stock_id
      LEFT JOIN {organism} momo ON momo.organism_id=mom.organism_id
      LEFT JOIN {organism} dado ON dado.organism_id=dad.organism_id";

    $where = [];
    $joins = [];

    // -- Remove RILL individuals.
    if($filter_results['show_individiuals']) {
      $where[] = 's.type_id IN (:type_id)';
      $args[':type_id'] = [3967, 6559];
    }
    else {
      $where[] = 's.type_id=:type_id';
      $args[':type_id'] = 3967;
    }

    // -- Genus.
    if ($filter_results['genus'] != '') {
      $where[] = "o.genus ~ :genus";
      $args[':genus'] = $filter_results['genus'];
    }

    // -- Species.
    if ($filter_results['species'] != '') {
      $where[] = "o.species ~ :species";
      $args[':species'] = $filter_results['species'];
    }

    // -- Parents.
    $this->addParentageQuery($filter_results, $where, $args);

    // -- Name.
    if ($filter_results['name'] != '') {
      $where[] = "(s.name ~* :name OR s.uniquename = :name)";
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
    $query .= "\n" . ' ORDER BY s.name ASC';

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
    parent::formatResults($form, $results);

    if (isset($form['results'])) {
      $header1 = [
        'name' => '',
        'mom' => [
          'data' => 'Maternal',
          'colspan' => 2,
          'style' => 'text-align: center',
        ],
        'dad' => [
          'data' => 'Paternal',
          'colspan' => 2,
          'style' => 'text-align: center',
        ],
      ];
      $header2 = $form['results']['#header'];
      foreach ($header2 as $key => $label) {
        $header2[$key] = [
          'data' => $label,
          'header' => TRUE,
        ];
      }

      $form['results']['#header'] = $header1;
      array_unshift($form['results']['#rows'], $header2);
    }
  }
}
