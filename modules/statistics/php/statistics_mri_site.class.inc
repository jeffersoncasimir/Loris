<?php declare(strict_types=1);

/**
 * The module to display study statistics
 *
 * PHP Version 7
 *
 * @category Loris
 * @package  Main
 * @author   Dave MacFarlane <driusan@bic.mni.mcgill.ca>
 * @license  http://www.gnu.org/licenses/gpl-3.0.txt GPLv3
 * @link     https://github.com/aces/Loris
 */
namespace LORIS\statistics;

/**
 * The module to display study statistics
 *
 * PHP Version 7
 *
 * @category Loris
 * @package  Main
 * @author   Dave MacFarlane <driusan@bic.mni.mcgill.ca>
 * @license  http://www.gnu.org/licenses/gpl-3.0.txt GPLv3
 * @link     https://github.com/aces/Loris
 */
class Statistics_Mri_Site extends Statistics_Site
{
    var $query_criteria = '';
    var $query_vars     = [];

    /**
     * CheckCriteria function
     *
     * @param ?\CenterID  $centerID  the value of centerID
     * @param ?\ProjectID $projectID the value of projectID
     *
     * @return void
     */
    function _checkCriteria(?\CenterID $centerID, ?\ProjectID $projectID)
    {
        if (!empty($centerID)) {
            $this->query_criteria   .= " AND s.CenterID =:cid ";
            $this->query_vars['cid'] = $centerID;
        }
        if (!empty($projectID)) {
            $this->query_criteria   .= " AND s.ProjectID =:pid ";
            $this->query_vars['pid'] = $projectID;
        }
    }
    /**
     * GetIssueName function
     *
     * @param string $issue the value of issue
     *
     * @return string
     */
    function _getIssueName(string $issue)
    {
        switch ($issue) {
        case "Tarchive_Missing":
            return "MRI Parameter Form Completed but Missing tarchive entry";
        case "PF_Missing":
            return "MRI Browser Populated, but no MRI parameter form completed";
        case "Files_Missing":
            return "MRI Parameter Form Completed but nothing in MRI Browser";
        }
        return $issue;
    }
    /**
     * CompleteCount function
     *
     * @param ?\CenterID  $centerID  the value of centerID
     * @param ?\ProjectID $projectID the value of projectID
     * @param string      $issue     the value of issue
     *
     * @return ?string
     */
    function _completeCount(
        ?\CenterID $centerID,
        ?\ProjectID $projectID,
        string $issue
    ): ?string {
        return null;
    }

    /**
     * GetResults function
     *
     * @param ?\CenterID  $centerID  the value of centerID
     * @param ?\ProjectID $projectID the value of projectID
     * @param string      $issue     the value of issue
     *
     * @return array
     */
    function _getResults(
        ?\CenterID $centerID,
        ?\ProjectID $projectID,
        string $issue
    ) {
        $this->_checkCriteria($centerID, $projectID);
        $DB = $this->loris->getDatabaseConnection();

        $scan_types = iterator_to_array(
            $DB->pselect(
                "SELECT MriScanTypeName AS ScanType from mri_scan_type",
                []
            )
        );
        $where      = "WHERE (";
        $counter    = 1;
        foreach (array_values($scan_types) as $scan) {
            foreach (array_values($scan) as $label) {
                if ($counter < sizeof($scan_types)) {
                    $where .= "m.$label <> 'No' OR m.$label IS NULL OR ";
                } else {
                    $where .= "m.$label <> 'No' OR m.$label IS NULL )";
                }
                $counter++;
            }
        }
        $query = '';
        switch ($issue) {
        case 'Tarchive_Missing':
            // TODO: MID() as-is may not work for longer int(10) CandIDs
            $query = "SELECT DISTINCT f.CommentID as CommentID,
                            c.PSCID, s.ID as SessionID, c.CandID as CandID,
                            s.Visit_label
                    FROM flag f JOIN session s ON (f.SessionID=s.ID)
                        JOIN test_names tn ON tn.ID = f.TestID
                        LEFT JOIN mri_parameter_form m ON (m.CommentID=f.CommentID)
                        LEFT JOIN candidate c ON (s.CandidateID=c.ID)
                        LEFT JOIN tarchive t ON (c.CandID=MID(t.PatientName, 9, 6)
                        AND MID(t.PatientName, 16, 7)=s.Visit_label)
                        $where
                            AND f.CommentID NOT LIKE 'DDE%'
                            AND tn.Test_name='mri_parameter_form'
                            AND f.Administration <> 'None'
                            AND t.TarchiveID IS NULL
                            AND s.Active='Y'
                            AND s.CenterID <> '1'
                            AND f.Data_entry='Complete'
                            $this->query_criteria
                        ORDER BY c.PSCID";
            break;
        case 'Files_Missing':
            $where   = "WHERE (";
            $counter = 1;
            foreach (array_values($scan_types) as $scan) {
                foreach (array_values($scan) as $label) {
                    if ($counter < sizeof($scan_types)) {
                        $where .= "m.$label <> 'No' OR ";
                    } else {
                        $where .= "m.$label <> 'No')";
                    }
                    $counter++;
                }
            }

            $query = "SELECT DISTINCT f.CommentID as CommentID,
                      s.ID as SessionID, c.PSCID, c.CandID, s.Visit_label
                    FROM mri_parameter_form m LEFT JOIN flag f ON
                                        (f.CommentID=m.CommentID)
                    LEFT JOIN session s ON (s.ID=f.SessionID)
                    LEFT JOIN files fi ON (fi.SessionID=f.SessionID)
                    LEFT JOIN candidate c ON (c.ID=s.CandidateID)
                    $where
                        AND fi.FileID IS NULL
                        AND f.Data_entry= 'Complete'
                        AND s.Active='Y'
                        AND s.CenterID <> '1'
                        $this->query_criteria
                    ORDER BY c.PSCID";
            break;
        case 'PF_Missing': $query = "SELECT DISTINCT c.PSCID, s.ID as SessionID,
                        s.Visit_label
                    FROM files LEFT JOIN session s ON (files.SessionID=s.ID)
                        LEFT JOIN flag f on
                       (f.SessionID=s.ID AND f.CommentID NOT LIKE 'DDE%')
                        LEFT JOIN test_names tn ON (tn.ID = f.TestID)
                        LEFT JOIN mri_parameter_form mpf
                       ON (mpf.CommentID=f.CommentID)
                        LEFT JOIN candidate c ON (c.ID=s.CandidateID)
                        WHERE s.Active='Y'
                           AND c.Entity_type != 'Scanner'
                           AND (f.ID IS NULL OR
                           f.Data_entry <> 'Complete' OR
                           f.Data_entry IS NULL)
                           AND s.CenterID <> '1'
                           AND tn.Test_name = 'mri_parameter_form'
                           $this->query_criteria
                           ORDER BY c.PSCID";
            break;

        }
        if ($query) {
            $result = $DB->pselect($query, $this->query_vars);
        }
        return $result ?? [];
    }

    /**
     * Setup function
     *
     * @return void
     */
    function setup()
    {
        $this->issues = [
            "Tarchive_Missing",
            "Files_Missing",
            "PF_Missing",
        ];

        $DB     = $this->loris->getDatabaseConnection();
        $sqlRow = "SELECT CenterID as ID,".
                  " PSCArea as Name FROM psc WHERE CenterID=:cid";
        $center = $DB->pselectRow($sqlRow, ['cid' => $_REQUEST['CenterID']]);
        if (is_null($center)) {
            throw new \LorisException('Invalid CenterID specified');
        }
        $id   = $center['ID'];
        $name = $center['Name'];
        // List of all visits. Add to it any time a new one is seen, so
        // that we can iterate over it to display later, and leave blank
        // cells for ones that are missing for a given issue in the
        // template
        $visits    = [];
        $projectID = isset($_REQUEST['ProjectID'])
            ? \ProjectID::singleton(intval($_REQUEST['ProjectID']))
            : null;

        $data = [];
        foreach ($this->issues as $issue) {
            // FIXME The function _completeCount() seems to be unfinished and
            // always returns null.
            $complete_count = $this->_completeCount($id, $projectID, $issue);

            $results = $this->_getResults($id, $projectID, $issue);

            $test_url = $issue;
            foreach ($results as $row) {
                if (!in_array($row['Visit_label'] ?? [], $visits)) {
                    $visits[] = $row['Visit_label'];
                }
                $results[$row['Visit_label']][] = [
                    'test_url'  => $test_url,
                    'CandID'    => $row['CandID'],
                    'SessionID' => $row['SessionID'],
                    'CommentID' => $row['CommentID'],
                    'PSCID'     => $row['PSCID'],
                ];
            }
            $data[] = [
                'name'        => $this->_getIssueName($issue),
                'count'       => $complete_count,
                'incompletes' => $results,
            ];
        }

        $this->tpl_data["data"]     = $data;
        $this->tpl_data["SiteName"] = $name;
        sort($visits);
        $this->tpl_data['AllVisits']      = $visits;
        $this->tpl_data['NumVisitLabels'] = count($visits);
    }

    /**
     * Include additional CSS files:
     *
     * @return array of javascript to be inserted
     */
    function getCSSDependencies()
    {
        $factory = \NDB_Factory::singleton();
        $baseURL = $factory->settings()->getBaseURL();
        $deps    = parent::getCSSDependencies();
        return array_merge(
            $deps,
            [$baseURL . "/statistics/css/statistics.css"]
        );
    }
}

